#include "log/RotationErrorHandler.h"
#include <thread>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <cstring>
#endif

/*****************************************************************************
 *  RotationErrorHandler
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     RotationErrorHandler.cpp
 *  @brief    轮转错误处理机制实现
 *  @details  提供重试、恢复和错误分类处理功能
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/16
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

RotationErrorHandler::RotationErrorHandler(const RetryConfig& retryConfig)
    : retryConfig_(retryConfig)
{
    InitializeDefaultErrorMapping();
}

RotationErrorHandler::~RotationErrorHandler() = default;

RecoveryResult RotationErrorHandler::HandleError(const RotationError& error,
                                                std::function<bool()> operation)
{
    RecoveryResult result;
    result.strategy = DetermineRecoveryStrategy(error);
    
    // 触发错误回调
    TriggerErrorCallback(error);
    
    // 更新统计信息
    UpdateErrorStatistics(error);
    
    auto startTime = std::chrono::steady_clock::now();
    
    try {
        switch (result.strategy) {
            case RecoveryStrategy::Retry:
                result = ExecuteRetry(operation, error);
                break;
                
            case RecoveryStrategy::Skip:
                result.success = true;
                result.message = L"Operation skipped due to error";
                break;
                
            case RecoveryStrategy::Fallback:
                // 尝试触发恢复回调
                result.success = TriggerRecoveryCallback(error, RecoveryStrategy::Fallback);
                result.message = result.success ? L"Fallback recovery succeeded" : L"Fallback recovery failed";
                break;
                
            case RecoveryStrategy::Manual:
                result.success = false;
                result.message = L"Manual intervention required";
                break;
                
            case RecoveryStrategy::Abort:
                result.success = false;
                result.message = L"Operation aborted due to critical error";
                break;
                
            default:
                result.success = false;
                result.message = L"No recovery strategy available";
                break;
        }
    }
    catch (const std::exception& ex) {
        result.success = false;
        result.message = std::wstring(ex.what(), ex.what() + strlen(ex.what()));
    }
    
    auto endTime = std::chrono::steady_clock::now();
    result.totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // 更新恢复统计
    UpdateRecoveryStatistics(result);
    
    return result;
}

RotationErrorType RotationErrorHandler::ClassifyError(int systemErrorCode, const std::wstring& context)
{
    // 首先检查自定义映射
    auto it = errorMapping_.find(systemErrorCode);
    if (it != errorMapping_.end()) {
        return it->second;
    }
    
    // 根据系统错误码分类
#ifdef _WIN32
    switch (systemErrorCode) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            return RotationErrorType::FileNotFound;
            
        case ERROR_ACCESS_DENIED:
            return RotationErrorType::Permissions;
            
        case ERROR_DISK_FULL:
        case ERROR_HANDLE_DISK_FULL:
            return RotationErrorType::DiskSpace;
            
        case ERROR_SHARING_VIOLATION:
        case ERROR_LOCK_VIOLATION:
            return RotationErrorType::FileLocked;
            
        case ERROR_NETWORK_UNREACHABLE:
        case ERROR_HOST_UNREACHABLE:
            return RotationErrorType::NetworkError;
            
        case ERROR_TIMEOUT:
            return RotationErrorType::Timeout;
            
        case ERROR_OPERATION_ABORTED:
            return RotationErrorType::UserCancelled;
            
        default:
            break;
    }
#else
    switch (systemErrorCode) {
        case ENOENT:
            return RotationErrorType::FileNotFound;
            
        case EACCES:
        case EPERM:
            return RotationErrorType::Permissions;
            
        case ENOSPC:
            return RotationErrorType::DiskSpace;
            
        case EBUSY:
        case ETXTBSY:
            return RotationErrorType::FileLocked;
            
        case ENETUNREACH:
        case EHOSTUNREACH:
            return RotationErrorType::NetworkError;
            
        case ETIMEDOUT:
            return RotationErrorType::Timeout;
            
        case ECANCELED:
            return RotationErrorType::UserCancelled;
            
        default:
            break;
    }
#endif
    
    // 根据上下文进一步分类
    if (context.find(L"compress") != std::wstring::npos) {
        return RotationErrorType::CompressionError;
    }
    
    if (context.find(L"config") != std::wstring::npos) {
        return RotationErrorType::ConfigurationError;
    }
    
    return RotationErrorType::Unknown;
}

ErrorSeverity RotationErrorHandler::AssessErrorSeverity(const RotationError& error)
{
    switch (error.type) {
        case RotationErrorType::SystemError:
        case RotationErrorType::ResourceExhausted:
            return ErrorSeverity::Critical;
            
        case RotationErrorType::DiskSpace:
        case RotationErrorType::Permissions:
        case RotationErrorType::ConfigurationError:
            return ErrorSeverity::High;
            
        case RotationErrorType::FileLocked:
        case RotationErrorType::NetworkError:
        case RotationErrorType::CompressionError:
        case RotationErrorType::Timeout:
            return ErrorSeverity::Medium;
            
        case RotationErrorType::FileNotFound:
        case RotationErrorType::UserCancelled:
            return ErrorSeverity::Low;
            
        default:
            return ErrorSeverity::Medium;
    }
}

RecoveryStrategy RotationErrorHandler::DetermineRecoveryStrategy(const RotationError& error)
{
    // 首先检查错误严重程度
    ErrorSeverity severity = AssessErrorSeverity(error);
    
    if (severity == ErrorSeverity::Critical) {
        return RecoveryStrategy::Abort;
    }
    
    // 根据错误类型确定策略
    switch (error.type) {
        case RotationErrorType::FileLocked:
        case RotationErrorType::NetworkError:
        case RotationErrorType::Timeout:
        case RotationErrorType::ResourceExhausted:
            return IsRetryableError(error.type) ? RecoveryStrategy::Retry : RecoveryStrategy::Manual;
            
        case RotationErrorType::FileNotFound:
            return RecoveryStrategy::Skip;
            
        case RotationErrorType::DiskSpace:
        case RotationErrorType::Permissions:
            return RecoveryStrategy::Manual;
            
        case RotationErrorType::CompressionError:
            return RecoveryStrategy::Fallback; // 可以跳过压缩
            
        case RotationErrorType::UserCancelled:
            return RecoveryStrategy::Abort;
            
        case RotationErrorType::ConfigurationError:
            return RecoveryStrategy::Manual;
            
        default:
            return RecoveryStrategy::Retry;
    }
}

RecoveryResult RotationErrorHandler::ExecuteRetry(std::function<bool()> operation, 
                                                 const RotationError& error)
{
    RecoveryResult result;
    result.strategy = RecoveryStrategy::Retry;
    
    size_t maxRetries = retryConfig_.maxRetries;
    size_t retryCount = 0;
    
    while (retryCount < maxRetries) {
        try {
            // 计算延迟时间
            auto delay = CalculateRetryDelay(retryCount);
            
            // 等待重试延迟
            if (delay.count() > 0) {
                WaitForRetryDelay(delay);
            }
            
            // 执行操作
            if (operation()) {
                result.success = true;
                result.retryCount = retryCount + 1;
                result.message = L"Operation succeeded after " + std::to_wstring(retryCount + 1) + L" attempts";
                return result;
            }
            
            retryCount++;
            
        } catch (const std::exception& ex) {
            RotationError retryError;
            retryError.type = RotationErrorType::Unknown;
            retryError.message = std::wstring(ex.what(), ex.what() + strlen(ex.what()));
            result.errors.push_back(retryError);
            
            retryCount++;
        }
    }
    
    result.success = false;
    result.retryCount = retryCount;
    result.message = L"Operation failed after " + std::to_wstring(retryCount) + L" attempts";
    
    return result;
}

RecoveryResult RotationErrorHandler::ExecuteRollback(std::function<bool()> rollbackOperation,
                                                    const RotationError& error)
{
    RecoveryResult result;
    result.strategy = RecoveryStrategy::Rollback;
    
    try {
        if (rollbackOperation()) {
            result.success = true;
            result.message = L"Rollback completed successfully";
        } else {
            result.success = false;
            result.message = L"Rollback operation failed";
        }
    } catch (const std::exception& ex) {
        result.success = false;
        result.message = L"Rollback failed with exception: " + 
                        std::wstring(ex.what(), ex.what() + strlen(ex.what()));
    }
    
    return result;
}

bool RotationErrorHandler::IsRetryableError(RotationErrorType errorType) const
{
    std::lock_guard<std::mutex> lock(configMutex_);
    
    auto& retryableErrors = retryConfig_.retryableErrors;
    return std::find(retryableErrors.begin(), retryableErrors.end(), errorType) != retryableErrors.end();
}

std::chrono::milliseconds RotationErrorHandler::CalculateRetryDelay(size_t retryCount) const
{
    std::lock_guard<std::mutex> lock(configMutex_);
    
    if (!retryConfig_.exponentialBackoff) {
        return retryConfig_.initialDelay;
    }
    
    // 指数退避算法
    auto delay = retryConfig_.initialDelay;
    for (size_t i = 0; i < retryCount; ++i) {
        auto newDelay = std::chrono::milliseconds(
            static_cast<long long>(delay.count() * retryConfig_.backoffMultiplier));
        
        if (newDelay > retryConfig_.maxDelay) {
            delay = retryConfig_.maxDelay;
            break;
        }
        delay = newDelay;
    }
    
    return delay;
}

void RotationErrorHandler::SetErrorCallback(ErrorCallback callback)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    errorCallback_ = callback;
}

void RotationErrorHandler::SetRecoveryCallback(RecoveryCallback callback)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    recoveryCallback_ = callback;
}

void RotationErrorHandler::SetRetryConfig(const RetryConfig& config)
{
    std::lock_guard<std::mutex> lock(configMutex_);
    retryConfig_ = config;
}

RetryConfig RotationErrorHandler::GetRetryConfig() const
{
    std::lock_guard<std::mutex> lock(configMutex_);
    return retryConfig_;
}

std::wstring RotationErrorHandler::GetErrorStatistics() const
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    std::wostringstream stats;
    stats << L"Error Handler Statistics:\n";
    stats << L"  Total Errors: " << totalErrors_ << L"\n";
    stats << L"  Total Recoveries: " << totalRecoveries_ << L"\n";
    stats << L"  Successful Recoveries: " << successfulRecoveries_ << L"\n";
    
    if (totalRecoveries_ > 0) {
        double successRate = static_cast<double>(successfulRecoveries_) / totalRecoveries_ * 100.0;
        stats << L"  Recovery Success Rate: " << std::fixed << std::setprecision(2) << successRate << L"%\n";
    }
    
    stats << L"\nError Type Breakdown:\n";
    for (const auto& pair : errorCounts_) {
        stats << L"  " << GetErrorTypeName(pair.first) << L": " << pair.second << L"\n";
    }
    
    stats << L"\nRecovery Strategy Usage:\n";
    for (const auto& pair : recoveryCounts_) {
        stats << L"  " << GetRecoveryStrategyName(pair.first) << L": " << pair.second << L"\n";
    }
    
    return stats.str();
}

void RotationErrorHandler::ResetStatistics()
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    errorCounts_.clear();
    recoveryCounts_.clear();
    totalErrors_ = 0;
    totalRecoveries_ = 0;
    successfulRecoveries_ = 0;
}

void RotationErrorHandler::AddErrorMapping(int systemErrorCode, RotationErrorType errorType)
{
    errorMapping_[systemErrorCode] = errorType;
}

bool RotationErrorHandler::RemoveErrorMapping(int systemErrorCode)
{
    auto it = errorMapping_.find(systemErrorCode);
    if (it != errorMapping_.end()) {
        errorMapping_.erase(it);
        return true;
    }
    return false;
}

std::wstring RotationErrorHandler::GetErrorTypeName(RotationErrorType errorType)
{
    switch (errorType) {
        case RotationErrorType::Unknown: return L"Unknown";
        case RotationErrorType::FileSystem: return L"FileSystem";
        case RotationErrorType::Permissions: return L"Permissions";
        case RotationErrorType::DiskSpace: return L"DiskSpace";
        case RotationErrorType::FileNotFound: return L"FileNotFound";
        case RotationErrorType::FileLocked: return L"FileLocked";
        case RotationErrorType::NetworkError: return L"NetworkError";
        case RotationErrorType::CompressionError: return L"CompressionError";
        case RotationErrorType::ConfigurationError: return L"ConfigurationError";
        case RotationErrorType::ResourceExhausted: return L"ResourceExhausted";
        case RotationErrorType::Timeout: return L"Timeout";
        case RotationErrorType::UserCancelled: return L"UserCancelled";
        case RotationErrorType::SystemError: return L"SystemError";
        default: return L"Unknown";
    }
}

std::wstring RotationErrorHandler::GetSeverityName(ErrorSeverity severity)
{
    switch (severity) {
        case ErrorSeverity::Low: return L"Low";
        case ErrorSeverity::Medium: return L"Medium";
        case ErrorSeverity::High: return L"High";
        case ErrorSeverity::Critical: return L"Critical";
        default: return L"Unknown";
    }
}

std::wstring RotationErrorHandler::GetRecoveryStrategyName(RecoveryStrategy strategy)
{
    switch (strategy) {
        case RecoveryStrategy::None: return L"None";
        case RecoveryStrategy::Retry: return L"Retry";
        case RecoveryStrategy::Skip: return L"Skip";
        case RecoveryStrategy::Fallback: return L"Fallback";
        case RecoveryStrategy::Rollback: return L"Rollback";
        case RecoveryStrategy::Manual: return L"Manual";
        case RecoveryStrategy::Abort: return L"Abort";
        default: return L"Unknown";
    }
}

// 私有方法实现

void RotationErrorHandler::InitializeDefaultErrorMapping()
{
#ifdef _WIN32
    errorMapping_[ERROR_FILE_NOT_FOUND] = RotationErrorType::FileNotFound;
    errorMapping_[ERROR_PATH_NOT_FOUND] = RotationErrorType::FileNotFound;
    errorMapping_[ERROR_ACCESS_DENIED] = RotationErrorType::Permissions;
    errorMapping_[ERROR_DISK_FULL] = RotationErrorType::DiskSpace;
    errorMapping_[ERROR_HANDLE_DISK_FULL] = RotationErrorType::DiskSpace;
    errorMapping_[ERROR_SHARING_VIOLATION] = RotationErrorType::FileLocked;
    errorMapping_[ERROR_LOCK_VIOLATION] = RotationErrorType::FileLocked;
    errorMapping_[ERROR_NETWORK_UNREACHABLE] = RotationErrorType::NetworkError;
    errorMapping_[ERROR_HOST_UNREACHABLE] = RotationErrorType::NetworkError;
    errorMapping_[ERROR_TIMEOUT] = RotationErrorType::Timeout;
    errorMapping_[ERROR_OPERATION_ABORTED] = RotationErrorType::UserCancelled;
#else
    errorMapping_[ENOENT] = RotationErrorType::FileNotFound;
    errorMapping_[EACCES] = RotationErrorType::Permissions;
    errorMapping_[EPERM] = RotationErrorType::Permissions;
    errorMapping_[ENOSPC] = RotationErrorType::DiskSpace;
    errorMapping_[EBUSY] = RotationErrorType::FileLocked;
    errorMapping_[ETXTBSY] = RotationErrorType::FileLocked;
    errorMapping_[ENETUNREACH] = RotationErrorType::NetworkError;
    errorMapping_[EHOSTUNREACH] = RotationErrorType::NetworkError;
    errorMapping_[ETIMEDOUT] = RotationErrorType::Timeout;
    errorMapping_[ECANCELED] = RotationErrorType::UserCancelled;
#endif
}

void RotationErrorHandler::TriggerErrorCallback(const RotationError& error)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (errorCallback_) {
        try {
            errorCallback_(error);
        } catch (...) {
            // 忽略回调中的异常
        }
    }
}

bool RotationErrorHandler::TriggerRecoveryCallback(const RotationError& error, RecoveryStrategy strategy)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (recoveryCallback_) {
        try {
            return recoveryCallback_(error, strategy);
        } catch (...) {
            // 回调异常时返回失败
            return false;
        }
    }
    return false;
}

void RotationErrorHandler::UpdateErrorStatistics(const RotationError& error)
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    totalErrors_++;
    errorCounts_[error.type]++;
}

void RotationErrorHandler::UpdateRecoveryStatistics(const RecoveryResult& result)
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    totalRecoveries_++;
    recoveryCounts_[result.strategy]++;
    
    if (result.success) {
        successfulRecoveries_++;
    }
}

void RotationErrorHandler::WaitForRetryDelay(std::chrono::milliseconds delay)
{
    std::this_thread::sleep_for(delay);
}

std::wstring RotationErrorHandler::BuildErrorContext(const RotationError& error) const
{
    std::wostringstream context;
    
    context << L"Error Type: " << GetErrorTypeName(error.type) << L"\n";
    context << L"Severity: " << GetSeverityName(error.severity) << L"\n";
    context << L"Message: " << error.message << L"\n";
    
    if (!error.fileName.empty()) {
        context << L"File: " << error.fileName << L"\n";
    }
    
    if (!error.operation.empty()) {
        context << L"Operation: " << error.operation << L"\n";
    }
    
    if (error.errorCode != 0) {
        context << L"Error Code: " << error.errorCode << L"\n";
    }
    
    if (!error.detailedMessage.empty()) {
        context << L"Details: " << error.detailedMessage << L"\n";
    }
    
    return context.str();
}

// 工厂类实现

std::unique_ptr<RotationErrorHandler> ErrorHandlerFactory::CreateStandard()
{
    RetryConfig config;
    config.maxRetries = 3;
    config.initialDelay = std::chrono::milliseconds(1000);
    config.maxDelay = std::chrono::milliseconds(10000);
    config.backoffMultiplier = 2.0;
    config.exponentialBackoff = true;
    
    return std::make_unique<RotationErrorHandler>(config);
}

std::unique_ptr<RotationErrorHandler> ErrorHandlerFactory::CreateFailFast()
{
    RetryConfig config;
    config.maxRetries = 0; // 不重试
    config.initialDelay = std::chrono::milliseconds(0);
    config.maxDelay = std::chrono::milliseconds(0);
    config.retryableErrors.clear(); // 清空可重试错误类型
    
    return std::make_unique<RotationErrorHandler>(config);
}

std::unique_ptr<RotationErrorHandler> ErrorHandlerFactory::CreateResilient()
{
    RetryConfig config;
    config.maxRetries = 10; // 更多重试次数
    config.initialDelay = std::chrono::milliseconds(500);
    config.maxDelay = std::chrono::milliseconds(30000);
    config.backoffMultiplier = 1.5;
    config.exponentialBackoff = true;
    
    // 添加更多可重试的错误类型
    config.retryableErrors = {
        RotationErrorType::FileSystem,
        RotationErrorType::DiskSpace,
        RotationErrorType::FileLocked,
        RotationErrorType::NetworkError,
        RotationErrorType::ResourceExhausted,
        RotationErrorType::Timeout,
        RotationErrorType::CompressionError,
        RotationErrorType::Unknown
    };
    
    return std::make_unique<RotationErrorHandler>(config);
}

std::unique_ptr<RotationErrorHandler> ErrorHandlerFactory::CreateCustom(const RetryConfig& config)
{
    return std::make_unique<RotationErrorHandler>(config);
}

// 工具类实现

RotationError ErrorHandlerUtils::CreateErrorFromException(const std::exception& ex,
                                                         const std::wstring& operation)
{
    RotationError error;
    error.type = RotationErrorType::Unknown;
    error.severity = ErrorSeverity::Medium;
    error.message = std::wstring(ex.what(), ex.what() + strlen(ex.what()));
    error.operation = operation;
    
    return error;
}

RotationError ErrorHandlerUtils::CreateErrorFromSystemCode(int errorCode,
                                                          const std::wstring& operation)
{
    RotationError error;
    error.errorCode = errorCode;
    error.operation = operation;
    
    // 使用错误处理器分类错误
    auto handler = ErrorHandlerFactory::CreateStandard();
    error.type = handler->ClassifyError(errorCode);
    error.severity = handler->AssessErrorSeverity(error);
    
#ifdef _WIN32
    // 获取Windows错误消息
    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&messageBuffer, 0, NULL);
    
    if (messageBuffer) {
        error.message = std::wstring(messageBuffer, size);
        LocalFree(messageBuffer);
    } else {
        error.message = L"System error code: " + std::to_wstring(errorCode);
    }
#else
    // 获取POSIX错误消息
    error.message = std::wstring(strerror(errorCode), strerror(errorCode) + strlen(strerror(errorCode)));
#endif
    
    return error;
}

std::wstring ErrorHandlerUtils::FormatErrorMessage(const RotationError& error)
{
    std::wostringstream formatted;
    
    formatted << L"[" << RotationErrorHandler::GetErrorTypeName(error.type) << L"] ";
    formatted << L"(" << RotationErrorHandler::GetSeverityName(error.severity) << L") ";
    formatted << error.message;
    
    if (!error.fileName.empty()) {
        formatted << L" [File: " << error.fileName << L"]";
    }
    
    if (!error.operation.empty()) {
        formatted << L" [Operation: " << error.operation << L"]";
    }
    
    if (error.errorCode != 0) {
        formatted << L" [Code: " << error.errorCode << L"]";
    }
    
    return formatted.str();
}

std::wstring ErrorHandlerUtils::FormatRecoveryResult(const RecoveryResult& result)
{
    std::wostringstream formatted;
    
    formatted << L"Recovery Strategy: " << RotationErrorHandler::GetRecoveryStrategyName(result.strategy);
    formatted << L", Success: " << (result.success ? L"Yes" : L"No");
    formatted << L", Time: " << result.totalTime.count() << L"ms";
    
    if (result.retryCount > 0) {
        formatted << L", Retries: " << result.retryCount;
    }
    
    if (!result.message.empty()) {
        formatted << L", Message: " << result.message;
    }
    
    return formatted.str();
}