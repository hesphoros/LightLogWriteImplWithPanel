#include "log/RotationPreChecker.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#endif

/*****************************************************************************
 *  RotationPreChecker
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     RotationPreChecker.cpp
 *  @brief    轮转预检查系统实现
 *  @details  在执行轮转前检查各种前置条件，防止轮转失败
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/16
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

RotationPreChecker::RotationPreChecker()
{
    InitializeDefaultChecks();
}

RotationPreChecker::~RotationPreChecker() = default;

PreCheckResult RotationPreChecker::CheckRotationConditions(const RotationCheckContext& context)
{
    PreCheckResult result;
    result.checkTime = std::chrono::system_clock::now();
    
    auto startTime = std::chrono::steady_clock::now();
    
    // 执行各项检查
    std::vector<std::function<CheckResult(const RotationCheckContext&)>> checks;
    
    if (IsCheckEnabled(CheckType::DiskSpace)) {
        checks.push_back([this](const RotationCheckContext& ctx) { return CheckDiskSpace(ctx); });
    }
    
    if (IsCheckEnabled(CheckType::FilePermissions)) {
        checks.push_back([this](const RotationCheckContext& ctx) { return CheckFilePermissions(ctx); });
    }
    
    if (IsCheckEnabled(CheckType::DirectoryAccess)) {
        checks.push_back([this](const RotationCheckContext& ctx) { return CheckDirectoryAccess(ctx); });
    }
    
    if (IsCheckEnabled(CheckType::FileExists)) {
        checks.push_back([this](const RotationCheckContext& ctx) { return CheckFileExists(ctx); });
    }
    
    if (IsCheckEnabled(CheckType::FileLocked)) {
        checks.push_back([this](const RotationCheckContext& ctx) { return CheckFileLocked(ctx); });
    }
    
    if (IsCheckEnabled(CheckType::ProcessPermissions)) {
        checks.push_back([this](const RotationCheckContext& ctx) { return CheckProcessPermissions(ctx); });
    }
    
    if (IsCheckEnabled(CheckType::SystemResources)) {
        checks.push_back([this](const RotationCheckContext& ctx) { return CheckSystemResources(ctx); });
    }
    
    // 执行自定义检查
    for (const auto& customCheck : customChecks_) {
        checks.push_back(customCheck.second);
    }
    
    // 执行所有检查
    result.totalChecks = checks.size();
    for (auto& check : checks) {
        CheckResult checkResult = ExecuteCheck(check, context);
        result.results.push_back(checkResult);
        
        if (checkResult.passed) {
            result.passedChecks++;
        }
        
        if (checkResult.severity == CheckSeverity::Warning) {
            result.hasWarnings = true;
        }
        
        if (checkResult.severity == CheckSeverity::Error || checkResult.severity == CheckSeverity::Critical) {
            result.hasErrors = true;
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    result.totalCheckTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // 确定是否可以执行轮转
    result.canRotate = !result.hasErrors && (result.passedChecks > 0);
    
    return result;
}

CheckResult RotationPreChecker::CheckDiskSpace(const RotationCheckContext& context)
{
    CheckResult result(CheckType::DiskSpace, CheckSeverity::Error, L"Disk Space Check", L"");
    
    try {
        std::wstring checkPath = context.archiveDirectory.empty() ? 
            std::filesystem::path(context.sourceFile).parent_path().wstring() : 
            context.archiveDirectory;
            
        uintmax_t availableSpace = GetAvailableDiskSpace(checkPath);
        size_t requiredSpace = context.estimatedFileSize;
        
        // 如果启用压缩，预估压缩后需要额外空间
        if (context.compressionEnabled) {
            requiredSpace += context.estimatedFileSize / 2; // 假设压缩比50%
        }
        
        // 如果需要备份，还需要额外空间
        if (context.createBackup) {
            requiredSpace += context.estimatedFileSize;
        }
        
        size_t thresholdBytes = diskSpaceThresholdMB_ * 1024 * 1024;
        size_t totalRequired = requiredSpace + thresholdBytes;
        
        if (availableSpace >= totalRequired) {
            result.passed = true;
            result.message = L"Sufficient disk space available";
        } else {
            result.passed = false;
            result.message = L"Insufficient disk space";
            result.suggestion = L"Free up disk space or change archive directory";
        }
        
        // 设置严重级别
        if (availableSpace < requiredSpace) {
            result.severity = CheckSeverity::Critical;
        } else if (availableSpace < totalRequired) {
            result.severity = CheckSeverity::Error;
        }
        
    } catch (const std::exception& ex) {
        result.passed = false;
        result.message = L"Failed to check disk space: " + std::wstring(ex.what(), ex.what() + strlen(ex.what()));
    }
    
    return result;
}

CheckResult RotationPreChecker::CheckFilePermissions(const RotationCheckContext& context)
{
    CheckResult result(CheckType::FilePermissions, CheckSeverity::Error, L"File Permissions Check", L"");
    
    try {
        bool sourceReadable = IsFileReadable(context.sourceFile);
        bool sourceWritable = IsFileWritable(context.sourceFile);
        
        if (!sourceReadable) {
            result.passed = false;
            result.message = L"Source file is not readable";
            result.suggestion = L"Check file permissions and ownership";
            return result;
        }
        
        if (!sourceWritable) {
            result.passed = false;
            result.message = L"Source file is not writable";
            result.suggestion = L"Check file permissions and ownership";
            return result;
        }
        
        // 检查目标目录是否可写
        std::wstring targetDir = std::filesystem::path(context.targetFile).parent_path().wstring();
        if (!IsDirectoryWritable(targetDir)) {
            result.passed = false;
            result.message = L"Target directory is not writable";
            result.suggestion = L"Check directory permissions";
            return result;
        }
        
        result.passed = true;
        result.message = L"File permissions are adequate";
        
    } catch (const std::exception& ex) {
        result.passed = false;
        result.message = L"Failed to check file permissions: " + std::wstring(ex.what(), ex.what() + strlen(ex.what()));
    }
    
    return result;
}

CheckResult RotationPreChecker::CheckDirectoryAccess(const RotationCheckContext& context)
{
    CheckResult result(CheckType::DirectoryAccess, CheckSeverity::Error, L"Directory Access Check", L"");
    
    try {
        namespace fs = std::filesystem;
        
        // 检查源文件目录
        std::wstring sourceDir = fs::path(context.sourceFile).parent_path().wstring();
        if (!fs::exists(sourceDir)) {
            result.passed = false;
            result.message = L"Source directory does not exist";
            return result;
        }
        
        // 检查归档目录
        if (!context.archiveDirectory.empty()) {
            if (!fs::exists(context.archiveDirectory)) {
                // 尝试创建归档目录
                try {
                    fs::create_directories(context.archiveDirectory);
                } catch (...) {
                    result.passed = false;
                    result.message = L"Cannot create archive directory";
                    result.suggestion = L"Check parent directory permissions";
                    return result;
                }
            }
            
            if (!IsDirectoryWritable(context.archiveDirectory)) {
                result.passed = false;
                result.message = L"Archive directory is not writable";
                result.suggestion = L"Check directory permissions";
                return result;
            }
        }
        
        result.passed = true;
        result.message = L"Directory access is adequate";
        
    } catch (const std::exception& ex) {
        result.passed = false;
        result.message = L"Failed to check directory access: " + std::wstring(ex.what(), ex.what() + strlen(ex.what()));
    }
    
    return result;
}

CheckResult RotationPreChecker::CheckFileExists(const RotationCheckContext& context)
{
    CheckResult result(CheckType::FileExists, CheckSeverity::Warning, L"File Existence Check", L"");
    
    try {
        namespace fs = std::filesystem;
        
        if (!fs::exists(context.sourceFile)) {
            result.passed = false;
            result.message = L"Source file does not exist";
            result.severity = CheckSeverity::Error;
            return result;
        }
        
        if (!context.targetFile.empty() && fs::exists(context.targetFile)) {
            result.passed = false;
            result.message = L"Target file already exists";
            result.severity = CheckSeverity::Warning;
            result.suggestion = L"Target file will be overwritten";
            return result;
        }
        
        result.passed = true;
        result.message = L"File existence check passed";
        
    } catch (const std::exception& ex) {
        result.passed = false;
        result.message = L"Failed to check file existence: " + std::wstring(ex.what(), ex.what() + strlen(ex.what()));
    }
    
    return result;
}

CheckResult RotationPreChecker::CheckFileLocked(const RotationCheckContext& context)
{
    CheckResult result(CheckType::FileLocked, CheckSeverity::Error, L"File Lock Check", L"");
    
    try {
        if (IsFileLocked(context.sourceFile)) {
            result.passed = false;
            result.message = L"Source file is locked by another process";
            result.suggestion = L"Close applications that may be using the file";
        } else {
            result.passed = true;
            result.message = L"File is not locked";
        }
        
    } catch (const std::exception& ex) {
        result.passed = false;
        result.message = L"Failed to check file lock: " + std::wstring(ex.what(), ex.what() + strlen(ex.what()));
    }
    
    return result;
}

CheckResult RotationPreChecker::CheckProcessPermissions(const RotationCheckContext& context)
{
    CheckResult result(CheckType::ProcessPermissions, CheckSeverity::Error, L"Process Permissions Check", L"");
    
    try {
#ifdef _WIN32
        // 检查是否有管理员权限（如果需要）
        BOOL isElevated = FALSE;
        HANDLE hToken = NULL;
        
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elevation;
            DWORD cbSize = sizeof(TOKEN_ELEVATION);
            
            if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize)) {
                isElevated = elevation.TokenIsElevated;
            }
            CloseHandle(hToken);
        }
        
        result.passed = true;
        result.message = isElevated ? L"Process has elevated permissions" : L"Process has standard permissions";
#else
        // 检查用户权限
        uid_t uid = getuid();
        if (uid == 0) {
            result.message = L"Process running as root";
        } else {
            result.message = L"Process running as regular user";
        }
        result.passed = true;
#endif
        
    } catch (const std::exception& ex) {
        result.passed = false;
        result.message = L"Failed to check process permissions: " + std::wstring(ex.what(), ex.what() + strlen(ex.what()));
    }
    
    return result;
}

CheckResult RotationPreChecker::CheckSystemResources(const RotationCheckContext& context)
{
    CheckResult result(CheckType::SystemResources, CheckSeverity::Warning, L"System Resources Check", L"");
    
    try {
        size_t memoryUsage = GetMemoryUsagePercent();
        
        if (memoryUsage > memoryThresholdPercent_) {
            result.passed = false;
            result.message = L"System memory usage is high: " + std::to_wstring(memoryUsage) + L"%";
            result.suggestion = L"Close unnecessary applications or increase memory";
            result.severity = memoryUsage > 90 ? CheckSeverity::Error : CheckSeverity::Warning;
        } else {
            result.passed = true;
            result.message = L"System resources are adequate";
        }
        
    } catch (const std::exception& ex) {
        result.passed = false;
        result.message = L"Failed to check system resources: " + std::wstring(ex.what(), ex.what() + strlen(ex.what()));
    }
    
    return result;
}

void RotationPreChecker::AddCustomCheck(const std::wstring& name, CustomCheckFunction checkFunction)
{
    customChecks_[name] = checkFunction;
}

bool RotationPreChecker::RemoveCustomCheck(const std::wstring& name)
{
    auto it = customChecks_.find(name);
    if (it != customChecks_.end()) {
        customChecks_.erase(it);
        return true;
    }
    return false;
}

void RotationPreChecker::SetDiskSpaceThreshold(size_t thresholdMB)
{
    diskSpaceThresholdMB_ = thresholdMB;
}

void RotationPreChecker::SetMemoryThreshold(size_t thresholdPercent)
{
    memoryThresholdPercent_ = (std::min)(thresholdPercent, static_cast<size_t>(100));
}

void RotationPreChecker::SetCheckTimeout(std::chrono::milliseconds timeout)
{
    checkTimeout_ = timeout;
}

void RotationPreChecker::SetCheckEnabled(CheckType checkType, bool enabled)
{
    checkEnabled_[checkType] = enabled;
}

bool RotationPreChecker::IsCheckEnabled(CheckType checkType) const
{
    auto it = checkEnabled_.find(checkType);
    return it != checkEnabled_.end() ? it->second : true;
}

std::wstring RotationPreChecker::GetCheckStatistics() const
{
    std::wostringstream stats;
    
    stats << L"Pre-Checker Statistics:\n";
    stats << L"  Total Checks: " << totalChecks_.load() << L"\n";
    stats << L"  Successful Checks: " << successfulChecks_.load() << L"\n";
    stats << L"  Failed Checks: " << failedChecks_.load() << L"\n";
    stats << L"  Average Check Time: ";
    
    if (totalChecks_.load() > 0) {
        double avgTime = static_cast<double>(totalCheckTimeMs_.load()) / totalChecks_.load();
        stats << std::fixed << std::setprecision(2) << avgTime << L"ms\n";
    } else {
        stats << L"N/A\n";
    }
    
    if (totalChecks_.load() > 0) {
        double successRate = static_cast<double>(successfulChecks_.load()) / totalChecks_.load() * 100.0;
        stats << L"  Success Rate: " << std::fixed << std::setprecision(2) << successRate << L"%\n";
    }
    
    return stats.str();
}

void RotationPreChecker::ResetStatistics()
{
    totalChecks_.store(0);
    successfulChecks_.store(0);
    failedChecks_.store(0);
    totalCheckTimeMs_.store(0);
}

// 私有方法实现

void RotationPreChecker::InitializeDefaultChecks()
{
    checkEnabled_[CheckType::DiskSpace] = true;
    checkEnabled_[CheckType::FilePermissions] = true;
    checkEnabled_[CheckType::DirectoryAccess] = true;
    checkEnabled_[CheckType::FileExists] = true;
    checkEnabled_[CheckType::FileLocked] = true;
    checkEnabled_[CheckType::ProcessPermissions] = false; // 默认关闭
    checkEnabled_[CheckType::SystemResources] = true;
    checkEnabled_[CheckType::NetworkAccess] = false; // 默认关闭
    checkEnabled_[CheckType::Custom] = true;
}

uintmax_t RotationPreChecker::GetAvailableDiskSpace(const std::wstring& path) const
{
    try {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::space_info space = fs::space(path, ec);
        
        if (ec) {
            return 0;
        }
        
        return space.available;
    }
    catch (...) {
        return 0;
    }
}

size_t RotationPreChecker::GetMemoryUsagePercent() const
{
#ifdef _WIN32
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    
    if (GlobalMemoryStatusEx(&memStatus)) {
        return static_cast<size_t>(memStatus.dwMemoryLoad);
    }
    return 0;
#else
    // 简化的Linux内存检查实现
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) {
        return 0;
    }
    
    std::string line;
    long long totalMem = 0, availableMem = 0;
    
    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            sscanf(line.c_str(), "MemTotal: %lld kB", &totalMem);
        } else if (line.find("MemAvailable:") == 0) {
            sscanf(line.c_str(), "MemAvailable: %lld kB", &availableMem);
        }
    }
    
    if (totalMem > 0 && availableMem >= 0) {
        return static_cast<size_t>((totalMem - availableMem) * 100 / totalMem);
    }
    return 0;
#endif
}

bool RotationPreChecker::IsFileReadable(const std::wstring& filePath) const
{
    try {
        std::ifstream file(filePath);
        return file.good();
    }
    catch (...) {
        return false;
    }
}

bool RotationPreChecker::IsFileWritable(const std::wstring& filePath) const
{
    try {
        namespace fs = std::filesystem;
        
        if (!fs::exists(filePath)) {
            return false;
        }
        
        // 尝试以追加模式打开文件
        std::ofstream file(filePath, std::ios::app);
        return file.good();
    }
    catch (...) {
        return false;
    }
}

bool RotationPreChecker::IsDirectoryWritable(const std::wstring& dirPath) const
{
    try {
        namespace fs = std::filesystem;
        
        if (!fs::exists(dirPath)) {
            return false;
        }
        
        // 尝试在目录中创建临时文件
        std::wstring tempFile = dirPath + L"/temp_write_test.tmp";
        std::ofstream file(tempFile);
        
        if (file.good()) {
            file.close();
            fs::remove(tempFile);
            return true;
        }
        
        return false;
    }
    catch (...) {
        return false;
    }
}

bool RotationPreChecker::IsFileLocked(const std::wstring& filePath) const
{
#ifdef _WIN32
    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, // 不允许共享
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        CloseHandle(hFile);
        return (error == ERROR_SHARING_VIOLATION || error == ERROR_LOCK_VIOLATION);
    }
    
    CloseHandle(hFile);
    return false;
#else
    int fd = open(filePath.c_str(), O_RDWR);
    if (fd == -1) {
        return (errno == EBUSY || errno == ETXTBSY);
    }
    
    // 尝试获取文件锁
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    
    int result = fcntl(fd, F_SETLK, &fl);
    close(fd);
    
    return (result == -1 && errno == EACCES);
#endif
}

CheckResult RotationPreChecker::ExecuteCheck(std::function<CheckResult(const RotationCheckContext&)> checkFunction,
                                           const RotationCheckContext& context)
{
    auto startTime = std::chrono::steady_clock::now();
    
    CheckResult result;
    try {
        result = checkFunction(context);
    }
    catch (const std::exception& ex) {
        result.passed = false;
        result.message = L"Check failed with exception: " + std::wstring(ex.what(), ex.what() + strlen(ex.what()));
        result.severity = CheckSeverity::Error;
    }
    
    auto endTime = std::chrono::steady_clock::now();
    result.checkDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    UpdateStatistics(result);
    
    return result;
}

void RotationPreChecker::UpdateStatistics(const CheckResult& result)
{
    totalChecks_.fetch_add(1);
    
    if (result.passed) {
        successfulChecks_.fetch_add(1);
    } else {
        failedChecks_.fetch_add(1);
    }
    
    totalCheckTimeMs_.fetch_add(result.checkDuration.count());
}

std::wstring RotationPreChecker::GenerateSuggestion(CheckType checkType, CheckSeverity severity) const
{
    switch (checkType) {
        case CheckType::DiskSpace:
            return L"Free up disk space or change archive directory";
        case CheckType::FilePermissions:
            return L"Check file permissions and ownership";
        case CheckType::DirectoryAccess:
            return L"Ensure directory exists and is accessible";
        case CheckType::FileLocked:
            return L"Close applications that may be using the file";
        case CheckType::SystemResources:
            return L"Close unnecessary applications or increase system resources";
        default:
            return L"Please check the system configuration";
    }
}

// PreCheckerFactory 实现

std::unique_ptr<RotationPreChecker> PreCheckerFactory::CreateStandard()
{
    auto checker = std::make_unique<RotationPreChecker>();
    checker->SetDiskSpaceThreshold(100); // 100MB
    checker->SetMemoryThreshold(80); // 80%
    return checker;
}

std::unique_ptr<RotationPreChecker> PreCheckerFactory::CreateFast()
{
    auto checker = std::make_unique<RotationPreChecker>();
    
    // 只启用关键检查
    checker->SetCheckEnabled(CheckType::DiskSpace, true);
    checker->SetCheckEnabled(CheckType::FileExists, true);
    checker->SetCheckEnabled(CheckType::FileLocked, true);
    checker->SetCheckEnabled(CheckType::FilePermissions, false);
    checker->SetCheckEnabled(CheckType::DirectoryAccess, false);
    checker->SetCheckEnabled(CheckType::ProcessPermissions, false);
    checker->SetCheckEnabled(CheckType::SystemResources, false);
    
    return checker;
}

std::unique_ptr<RotationPreChecker> PreCheckerFactory::CreateComprehensive()
{
    auto checker = std::make_unique<RotationPreChecker>();
    
    // 启用所有检查
    checker->SetCheckEnabled(CheckType::DiskSpace, true);
    checker->SetCheckEnabled(CheckType::FilePermissions, true);
    checker->SetCheckEnabled(CheckType::DirectoryAccess, true);
    checker->SetCheckEnabled(CheckType::FileExists, true);
    checker->SetCheckEnabled(CheckType::FileLocked, true);
    checker->SetCheckEnabled(CheckType::ProcessPermissions, true);
    checker->SetCheckEnabled(CheckType::SystemResources, true);
    checker->SetCheckEnabled(CheckType::NetworkAccess, true);
    
    // 设置更严格的阈值
    checker->SetDiskSpaceThreshold(500); // 500MB
    checker->SetMemoryThreshold(70); // 70%
    
    return checker;
}

std::unique_ptr<RotationPreChecker> PreCheckerFactory::CreateCustom(const std::vector<CheckType>& enabledChecks)
{
    auto checker = std::make_unique<RotationPreChecker>();
    
    // 先禁用所有检查
    for (int i = static_cast<int>(CheckType::DiskSpace); i <= static_cast<int>(CheckType::Custom); ++i) {
        checker->SetCheckEnabled(static_cast<CheckType>(i), false);
    }
    
    // 启用指定的检查
    for (CheckType checkType : enabledChecks) {
        checker->SetCheckEnabled(checkType, true);
    }
    
    return checker;
}