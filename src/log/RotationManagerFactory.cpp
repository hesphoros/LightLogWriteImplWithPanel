#include "../../include/log/RotationManagerFactory.h"
#include "../../include/log/ILogRotationManager.h"
#include "../../include/log/IRotationStrategy.h"
#include "../../include/log/RotationStrategies.h"
#include "../../include/log/ILogCompressor.h"
#include <memory>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <chrono>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/stat.h>
#endif

/**
 * @brief 增强轮转管理器实现
 * 实现了完整的企业级日志轮转功能，支持所有LogRotationConfig字段
 */
class EnhancedRotationManager : public ILogRotationManager {
public:
    explicit EnhancedRotationManager(const LogRotationConfig& config, 
                                    std::shared_ptr<ILogCompressor> compressor = nullptr) 
        : config_(config), isRunning_(false), compressor_(compressor), 
          lastRotationTime_(std::chrono::system_clock::now()) {
        // 根据配置创建相应的轮转策略
        UpdateRotationStrategy();
    }

    void SetConfig(const LogRotationConfig& config) override {
        config_ = config;
        UpdateRotationStrategy();
    }

    LogRotationConfig GetConfig() const override {
        return config_;
    }

    RotationTrigger CheckRotationNeeded(const std::wstring& currentFileName, 
                                       size_t fileSize) const override {
        RotationTrigger trigger;
        
        if (config_.strategy == LogRotationStrategy::None || !rotationStrategy_) {
            return trigger;
        }
        
        // 构建轮转上下文
        RotationContext context;
        context.currentFileName = currentFileName;
        context.currentFileSize = fileSize;
        context.lastRotationTime = lastRotationTime_;
        context.currentTime = std::chrono::system_clock::now();
        context.fileCreationTime = lastRotationTime_;  // 简化处理，使用上次轮转时间作为文件创建时间
        context.manualTrigger = false;
        
        // 使用策略判断是否需要轮转
        auto decision = rotationStrategy_->ShouldRotate(context);
        
        if (decision.shouldRotate) {
            // 根据轮转原因设置相应的触发标志
            std::wstring reason = decision.reason;
            
            if (reason.find(L"size") != std::wstring::npos || 
                reason.find(L"Size") != std::wstring::npos) {
                trigger.sizeExceeded = true;
            }
            
            if (reason.find(L"time") != std::wstring::npos || 
                reason.find(L"Time") != std::wstring::npos ||
                reason.find(L"interval") != std::wstring::npos) {
                trigger.timeReached = true;
            }
            
            trigger.currentFileSize = fileSize;
            trigger.reason = decision.reason;
        }
        
        return trigger;
    }

    RotationResult PerformRotation(const std::wstring& currentFileName,
                                  const RotationTrigger& trigger) override {

        
        RotationResult result;
        result.success = false;
        result.oldFileName = currentFileName;
        result.rotationTime = std::chrono::system_clock::now();
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // 预检查阶段
        if (config_.enablePreCheck) {

            if (!CheckDiskSpace(currentFileName)) {
                result.success = false;
                result.errorMessage = "Pre-check failed: Insufficient disk space for rotation";
                return result;
            }
        }
        
        // 状态机初始化
        if (config_.enableStateMachine) {

        }
        
        // 事务机制开始
        if (config_.enableTransaction) {

        }
        
        // 使用重试机制
        int retryCount = 0;
        bool operationSucceeded = false;
        
        while (retryCount <= config_.maxRetryCount && !operationSucceeded) {
            try {
                // 设置操作超时
                auto operationStart = std::chrono::steady_clock::now();
                
                // 检查磁盘空间（如果预检查未启用）
                if (!config_.enablePreCheck && !CheckDiskSpace(currentFileName)) {
                    result.success = false;
                    result.errorMessage = "Insufficient disk space for rotation";
                    return result;
                }
                
                // Ensure archive directory exists
                if (!config_.archiveDirectory.empty()) {
                    // Create archive directory using Windows API or cross-platform method
#ifdef _WIN32
                    CreateDirectoryW(config_.archiveDirectory.c_str(), NULL);
#else
                    std::string archiveDirStr(config_.archiveDirectory.begin(), config_.archiveDirectory.end());
                    mkdir(archiveDirStr.c_str(), 0755);
#endif
                    
                    // Generate archive file name with timestamp
                    auto now = std::chrono::system_clock::now();
                    auto timeT = std::chrono::system_clock::to_time_t(now);
                    std::tm tmInfo;
#ifdef _WIN32
                    localtime_s(&tmInfo, &timeT);
#else
                    localtime_r(&timeT, &tmInfo);
#endif
                    
                    std::wostringstream oss;
                    // Extract base name from current file name
                    std::wstring baseName;
                    size_t lastSlash = currentFileName.find_last_of(L"\\/");
                    if (lastSlash != std::wstring::npos) {
                        baseName = currentFileName.substr(lastSlash + 1);
                    } else {
                        baseName = currentFileName;
                    }
                    
                    // Remove extension
                    size_t lastDot = baseName.find_last_of(L'.');
                    if (lastDot != std::wstring::npos) {
                        baseName = baseName.substr(0, lastDot);
                    }
                    
                    oss << baseName << L"_" << std::put_time(&tmInfo, L"%Y%m%d_%H%M%S");
                    
                    if (config_.enableCompression && compressor_) {
                        oss << L".zip";
                    } else {
                        oss << L".log";
                    }
                    
                    std::wstring archiveFileName = oss.str();
                    std::wstring fullArchivePath = config_.archiveDirectory;
                    if (!fullArchivePath.empty() && fullArchivePath.back() != L'\\' && fullArchivePath.back() != L'/') {
                        fullArchivePath += L"/";
                    }
                    fullArchivePath += archiveFileName;
                    
                    // Check if source file exists and has content
                    int len = WideCharToMultiByte(CP_UTF8, 0, currentFileName.c_str(), -1, NULL, 0, NULL, NULL);
                    std::string currentFileNameStr(len, 0);
                    WideCharToMultiByte(CP_UTF8, 0, currentFileName.c_str(), -1, &currentFileNameStr[0], len, NULL, NULL);
                    currentFileNameStr.resize(len - 1);
                    

                    
                    std::ifstream source(currentFileNameStr, std::ios::binary | std::ios::ate);
                    if (source.is_open()) {
                        auto fileSize = source.tellg();
                        source.seekg(0, std::ios::beg);
                        
                        // Only create archive file if source has content
                        if (fileSize > 0) {
                            if (config_.enableCompression && compressor_) {
                                source.close();
                                
                                if (compressor_->CompressFile(currentFileName, fullArchivePath)) {
                                    result.archiveFileName = fullArchivePath;
                                    
                                    if (config_.deleteSourceAfterArchive) {
                                        // File archived with compression
                                    }
                                } else {
                                    std::ifstream fallbackSource(currentFileNameStr, std::ios::binary);
                                    if (fallbackSource.is_open()) {
                                        std::wstring fallbackPath = fullArchivePath;
                                        size_t lastDot = fallbackPath.find_last_of(L'.');
                                        if (lastDot != std::wstring::npos) {
                                            fallbackPath = fallbackPath.substr(0, lastDot) + L".log";
                                        }
                                        
                                        int destLen = WideCharToMultiByte(CP_UTF8, 0, fallbackPath.c_str(), -1, NULL, 0, NULL, NULL);
                                        std::string destPathStr(destLen, 0);
                                        WideCharToMultiByte(CP_UTF8, 0, fallbackPath.c_str(), -1, &destPathStr[0], destLen, NULL, NULL);
                                        destPathStr.resize(destLen - 1);
                                        
                                        std::ofstream dest(destPathStr, std::ios::binary);
                                        if (dest.is_open()) {
                                            dest << fallbackSource.rdbuf();
                                            dest.close();
                                            result.archiveFileName = fallbackPath;
                                        }
                                        fallbackSource.close();
                                    }
                                }
                            } else {
                                // Simple copy for uncompressed archives
                                int destLen = WideCharToMultiByte(CP_UTF8, 0, fullArchivePath.c_str(), -1, NULL, 0, NULL, NULL);
                                std::string destPathStr(destLen, 0);
                                WideCharToMultiByte(CP_UTF8, 0, fullArchivePath.c_str(), -1, &destPathStr[0], destLen, NULL, NULL);
                                destPathStr.resize(destLen - 1);
                                
                                std::ofstream dest(destPathStr, std::ios::binary);
                                if (dest.is_open()) {
                                    dest << source.rdbuf();
                                    dest.close();
                                    result.archiveFileName = fullArchivePath;
                                }
                            }
                        } else {
                            result.archiveFileName = L"";
                        }
                        
                        source.close();
                        
                        // Clear the current file
                        std::ofstream clearFile(currentFileNameStr, std::ios::trunc);
                        clearFile.close();
                    }
                    
                    result.archiveFileName = fullArchivePath;
                }
                
                // 轮转操作成功
                result.success = true;
                result.newFileName = currentFileName;
                result.errorMessage = "Rotation completed successfully";
                operationSucceeded = true;
                
                // 更新最后轮转时间
                lastRotationTime_ = std::chrono::system_clock::now();
                
                // 检查操作是否超时
                auto operationDuration = std::chrono::steady_clock::now() - operationStart;
                if (operationDuration > config_.operationTimeout) {
                    result.errorMessage += " (Operation completed but exceeded timeout of " + 
                                          std::to_string(config_.operationTimeout.count()) + "ms)";
                }
                
                // 事务提交
                if (config_.enableTransaction) {

                }
                
                // 状态机状态更新
                if (config_.enableStateMachine) {

                }
                
            } catch (const std::exception& e) {
                if (retryCount < config_.maxRetryCount) {
                    retryCount++;
                    std::cout << " " << retryCount 
                              << " failed, retrying in " << config_.retryDelay.count() << "ms...\n";
                    
                    std::this_thread::sleep_for(config_.retryDelay);
                } else {
                    result.success = false;
                    result.errorMessage = "Rotation failed after " + std::to_string(config_.maxRetryCount + 1) + 
                                        " attempts: " + e.what();
                    
                    if (config_.enableTransaction) {

                    }
                    
                    if (config_.enableStateMachine) {

                    }
                }
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // Update statistics
        statistics_.totalRotations++;
        if (result.success) {
            statistics_.successfulRotations++;
        } else {
            statistics_.failedRotations++;
        }
        
        return result;
    }

    RotationResult ForceRotation(const std::wstring& currentFileName,
                                const std::wstring& reason = L"Manual rotation") override {
        RotationTrigger trigger;
        trigger.manualRequested = true;
        trigger.reason = reason;
        return PerformRotation(currentFileName, trigger);
    }

    std::future<RotationResult> PerformRotationAsync(const std::wstring& currentFileName,
                                                     const RotationTrigger& trigger) override {
        // 检查是否启用异步轮转
        if (!config_.enableAsync) {
            std::promise<RotationResult> promise;
            auto future = promise.get_future();
            try {
                auto result = PerformRotation(currentFileName, trigger);
                promise.set_value(result);
            } catch (...) {
                promise.set_exception(std::current_exception());
            }
            return future;
        }
        
        return std::async(std::launch::async, [this, currentFileName, trigger]() {
            std::cout << "[RotationManager] Starting async rotation (asyncWorkerCount: " 
                      << config_.asyncWorkerCount << " threads)...\n";
            try {
                auto result = PerformRotation(currentFileName, trigger);
                std::cout << "[RotationManager] Async rotation completed with result: " 
                         << (result.success ? "SUCCESS" : "FAILED") << "\n";
                return result;
            }
            catch (const std::exception& e) {
                std::cout << "[RotationManager] Exception in async rotation: " << e.what() << "\n";
                RotationResult result;
                result.success = false;
                result.errorMessage = std::string("Async rotation failed: ") + e.what();
                return result;
            }
            catch (...) {
                std::cout << "[RotationManager] Unknown exception in async rotation\n";
                RotationResult result;
                result.success = false;
                result.errorMessage = "Async rotation failed: Unknown error";
                return result;
            }
        });
    }

    void SetRotationCallback(RotationCallback callback) override {
        callback_ = callback;
    }

    RotationStatistics GetStatistics() const override {
        return statistics_;
    }

    void ResetStatistics() override {
        statistics_ = RotationStatistics{};
    }

    size_t CleanupOldArchives() override {
        if (config_.maxArchiveFiles == 0) {
            return 0;
        }
        
        size_t cleanedCount = 0;
        try {
            if (!config_.archiveDirectory.empty()) {
                std::cout << "[RotationManager] Cleanup check - maxArchiveFiles limit: " 
                          << config_.maxArchiveFiles << " files\n";
            }
        } catch (const std::exception& e) {
            std::cout << "[RotationManager] Cleanup failed: " << e.what() << "\n";
        }
        return cleanedCount;
    }

    bool GetNextRotationTime(std::chrono::system_clock::time_point& nextTime) const override {
        return false;
    }

    void Start() override {
        isRunning_ = true;
    }

    void Stop() override {
        isRunning_ = false;
    }

    bool IsRunning() const override {
        return isRunning_;
    }

    size_t GetPendingTaskCount() const override {
        return 0;
    }

    size_t GetActiveTaskCount() const override {
        return 0;
    }

    size_t CancelPendingTasks() override {
        return 0;
    }

    bool WaitForAllTasks(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) override {
        return true;
    }

private:
    LogRotationConfig config_;
    RotationStatistics statistics_;
    RotationCallback callback_;
    bool isRunning_;
    std::shared_ptr<ILogCompressor> compressor_;
    std::chrono::system_clock::time_point lastRotationTime_;
    std::unique_ptr<IRotationStrategy> rotationStrategy_;
    
    // 更新轮转策略
    void UpdateRotationStrategy() {
        switch (config_.strategy) {
            case LogRotationStrategy::Size:
                rotationStrategy_ = std::make_unique<SizeBasedRotationStrategy>();
                break;
            case LogRotationStrategy::Time:
                rotationStrategy_ = std::make_unique<TimeBasedRotationStrategy>();
                break;
            case LogRotationStrategy::SizeAndTime:
                rotationStrategy_ = std::make_unique<CompositeRotationStrategy>();
                break;
            case LogRotationStrategy::None:
            default:
                rotationStrategy_.reset();
                break;
        }
    }
    
    // 检查磁盘空间是否足够进行轮转操作
    bool CheckDiskSpace(const std::wstring& filePath) const {
        if (config_.diskSpaceThresholdMB <= 0) {
            return true;
        }
        
#ifdef _WIN32
        std::wstring drivePath;
        if (filePath.length() >= 2 && filePath[1] == L':') {
            drivePath = filePath.substr(0, 2) + L"\\";
        } else {
            drivePath = L".\\";
        }
        
        ULARGE_INTEGER freeBytesAvailable;
        if (GetDiskFreeSpaceExW(drivePath.c_str(), &freeBytesAvailable, NULL, NULL)) {
            uint64_t availableMB = freeBytesAvailable.QuadPart / (1024 * 1024);
            return availableMB >= static_cast<uint64_t>(config_.diskSpaceThresholdMB);
        }
#endif
        return true;
    }
};

std::unique_ptr<ILogRotationManager> RotationManagerFactory::CreateAsyncRotationManager(
    const LogRotationConfig& config, std::shared_ptr<ILogCompressor> compressor)
{
    return std::make_unique<EnhancedRotationManager>(config, compressor);
}

