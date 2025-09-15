#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <memory>
#include <map>
#include <mutex>
#include <atomic>

/*****************************************************************************
 *  RotationErrorHandler
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     RotationErrorHandler.h
 *  @brief    轮转错误处理机制
 *  @details  提供重试、恢复和错误分类处理功能
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/15
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

/**
 * @brief 错误类型分类
 */
enum class RotationErrorType {
    Unknown,            /*!< 未知错误 */
    FileSystem,         /*!< 文件系统错误 */
    Permissions,        /*!< 权限错误 */
    DiskSpace,          /*!< 磁盘空间不足 */
    FileNotFound,       /*!< 文件未找到 */
    FileLocked,         /*!< 文件被锁定 */
    NetworkError,       /*!< 网络错误 */
    CompressionError,   /*!< 压缩错误 */
    ConfigurationError, /*!< 配置错误 */
    ResourceExhausted,  /*!< 资源耗尽 */
    Timeout,            /*!< 超时错误 */
    UserCancelled,      /*!< 用户取消 */
    SystemError         /*!< 系统错误 */
};

/**
 * @brief 错误严重程度
 */
enum class ErrorSeverity {
    Low,        /*!< 低严重程度 - 可继续操作 */
    Medium,     /*!< 中等严重程度 - 需要注意 */
    High,       /*!< 高严重程度 - 需要干预 */
    Critical    /*!< 严重错误 - 系统无法继续 */
};

/**
 * @brief 恢复策略
 */
enum class RecoveryStrategy {
    None,           /*!< 不进行恢复 */
    Retry,          /*!< 重试操作 */
    Skip,           /*!< 跳过当前操作 */
    Fallback,       /*!< 使用备用方案 */
    Rollback,       /*!< 回滚操作 */
    Manual,         /*!< 需要人工干预 */
    Abort           /*!< 中止所有操作 */
};

/**
 * @brief 轮转错误信息
 */
struct RotationError {
    RotationErrorType type = RotationErrorType::Unknown;    /*!< 错误类型 */
    ErrorSeverity severity = ErrorSeverity::Medium;        /*!< 严重程度 */
    std::wstring message;                                   /*!< 错误消息 */
    std::wstring detailedMessage;                           /*!< 详细错误消息 */
    std::wstring fileName;                                  /*!< 相关文件名 */
    std::wstring operation;                                 /*!< 执行的操作 */
    std::chrono::system_clock::time_point timestamp;       /*!< 错误时间 */
    int errorCode = 0;                                      /*!< 系统错误码 */
    std::wstring stackTrace;                                /*!< 调用栈（如果可用） */
    std::map<std::wstring, std::wstring> metadata;         /*!< 额外元数据 */
    
    RotationError() : timestamp(std::chrono::system_clock::now()) {}
    
    RotationError(RotationErrorType t, const std::wstring& msg)
        : type(t), message(msg), timestamp(std::chrono::system_clock::now()) {}
    
    RotationError(RotationErrorType t, ErrorSeverity s, const std::wstring& msg)
        : type(t), severity(s), message(msg), timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief 重试配置
 */
struct RetryConfig {
    size_t maxRetries = 3;                                  /*!< 最大重试次数 */
    std::chrono::milliseconds initialDelay{1000};          /*!< 初始延迟 */
    std::chrono::milliseconds maxDelay{10000};             /*!< 最大延迟 */
    double backoffMultiplier = 2.0;                        /*!< 退避乘数 */
    bool exponentialBackoff = true;                         /*!< 是否使用指数退避 */
    std::vector<RotationErrorType> retryableErrors;        /*!< 可重试的错误类型 */
    
    RetryConfig() {
        // 默认可重试的错误类型
        retryableErrors = {
            RotationErrorType::FileSystem,
            RotationErrorType::DiskSpace,
            RotationErrorType::FileLocked,
            RotationErrorType::NetworkError,
            RotationErrorType::ResourceExhausted,
            RotationErrorType::Timeout
        };
    }
};

/**
 * @brief 恢复结果
 */
struct RecoveryResult {
    bool success = false;                                   /*!< 恢复是否成功 */
    RecoveryStrategy strategy = RecoveryStrategy::None;     /*!< 使用的恢复策略 */
    std::wstring message;                                   /*!< 恢复消息 */
    size_t retryCount = 0;                                  /*!< 重试次数 */
    std::chrono::milliseconds totalTime{0};                /*!< 总恢复时间 */
    std::vector<RotationError> errors;                     /*!< 恢复过程中的错误 */
    
    RecoveryResult() = default;
    
    RecoveryResult(bool succ, RecoveryStrategy strat, const std::wstring& msg)
        : success(succ), strategy(strat), message(msg) {}
};

/**
 * @brief 错误处理回调函数类型
 */
using ErrorCallback = std::function<void(const RotationError&)>;
using RecoveryCallback = std::function<bool(const RotationError&, RecoveryStrategy)>;

/**
 * @brief 轮转错误处理器
 * @details 提供错误分类、重试、恢复等功能
 */
class RotationErrorHandler {
public:
    /**
     * @brief 构造函数
     * @param retryConfig 重试配置
     */
    explicit RotationErrorHandler(const RetryConfig& retryConfig = RetryConfig{});
    
    /**
     * @brief 析构函数
     */
    ~RotationErrorHandler();
    
    /**
     * @brief 处理轮转错误
     * @param error 错误信息
     * @param operation 失败的操作函数
     * @return 恢复结果
     */
    RecoveryResult HandleError(const RotationError& error,
                              std::function<bool()> operation);
    
    /**
     * @brief 分类错误类型
     * @param systemErrorCode 系统错误码
     * @param context 错误上下文
     * @return 错误类型
     */
    RotationErrorType ClassifyError(int systemErrorCode, const std::wstring& context = L"");
    
    /**
     * @brief 评估错误严重程度
     * @param error 错误信息
     * @return 严重程度
     */
    ErrorSeverity AssessErrorSeverity(const RotationError& error);
    
    /**
     * @brief 确定恢复策略
     * @param error 错误信息
     * @return 恢复策略
     */
    RecoveryStrategy DetermineRecoveryStrategy(const RotationError& error);
    
    /**
     * @brief 执行重试操作
     * @param operation 要重试的操作
     * @param error 原始错误
     * @return 恢复结果
     */
    RecoveryResult ExecuteRetry(std::function<bool()> operation, const RotationError& error);
    
    /**
     * @brief 执行回滚操作
     * @param rollbackOperation 回滚操作函数
     * @param error 原始错误
     * @return 恢复结果
     */
    RecoveryResult ExecuteRollback(std::function<bool()> rollbackOperation,
                                  const RotationError& error);
    
    /**
     * @brief 检查错误是否可重试
     * @param errorType 错误类型
     * @return 是否可重试
     */
    bool IsRetryableError(RotationErrorType errorType) const;
    
    /**
     * @brief 计算重试延迟
     * @param retryCount 当前重试次数
     * @return 延迟时间
     */
    std::chrono::milliseconds CalculateRetryDelay(size_t retryCount) const;
    
    /**
     * @brief 设置错误回调
     * @param callback 错误回调函数
     */
    void SetErrorCallback(ErrorCallback callback);
    
    /**
     * @brief 设置恢复回调
     * @param callback 恢复回调函数
     */
    void SetRecoveryCallback(RecoveryCallback callback);
    
    /**
     * @brief 设置重试配置
     * @param config 重试配置
     */
    void SetRetryConfig(const RetryConfig& config);
    
    /**
     * @brief 获取重试配置
     * @return 重试配置
     */
    RetryConfig GetRetryConfig() const;
    
    /**
     * @brief 获取错误统计信息
     * @return 统计信息字符串
     */
    std::wstring GetErrorStatistics() const;
    
    /**
     * @brief 重置错误统计
     */
    void ResetStatistics();
    
    /**
     * @brief 添加自定义错误映射
     * @param systemErrorCode 系统错误码
     * @param errorType 错误类型
     */
    void AddErrorMapping(int systemErrorCode, RotationErrorType errorType);
    
    /**
     * @brief 移除错误映射
     * @param systemErrorCode 系统错误码
     * @return 是否移除成功
     */
    bool RemoveErrorMapping(int systemErrorCode);
    
    /**
     * @brief 获取错误类型名称
     * @param errorType 错误类型
     * @return 类型名称
     */
    static std::wstring GetErrorTypeName(RotationErrorType errorType);
    
    /**
     * @brief 获取严重程度名称  
     * @param severity 严重程度
     * @return 严重程度名称
     */
    static std::wstring GetSeverityName(ErrorSeverity severity);
    
    /**
     * @brief 获取恢复策略名称
     * @param strategy 恢复策略
     * @return 策略名称
     */
    static std::wstring GetRecoveryStrategyName(RecoveryStrategy strategy);

private:
    // 配置
    mutable std::mutex configMutex_;
    RetryConfig retryConfig_;
    
    // 错误映射
    std::map<int, RotationErrorType> errorMapping_;
    
    // 回调
    mutable std::mutex callbackMutex_;
    ErrorCallback errorCallback_;
    RecoveryCallback recoveryCallback_;
    
    // 统计信息
    mutable std::mutex statsMutex_;
    std::map<RotationErrorType, size_t> errorCounts_;
    std::map<RecoveryStrategy, size_t> recoveryCounts_;
    size_t totalErrors_ = 0;
    size_t totalRecoveries_ = 0;
    size_t successfulRecoveries_ = 0;
    
    /**
     * @brief 初始化默认错误映射
     */
    void InitializeDefaultErrorMapping();
    
    /**
     * @brief 触发错误回调
     * @param error 错误信息
     */
    void TriggerErrorCallback(const RotationError& error);
    
    /**
     * @brief 触发恢复回调
     * @param error 错误信息
     * @param strategy 恢复策略
     * @return 回调是否成功处理
     */
    bool TriggerRecoveryCallback(const RotationError& error, RecoveryStrategy strategy);
    
    /**
     * @brief 更新错误统计
     * @param error 错误信息
     */
    void UpdateErrorStatistics(const RotationError& error);
    
    /**
     * @brief 更新恢复统计
     * @param result 恢复结果
     */
    void UpdateRecoveryStatistics(const RecoveryResult& result);
    
    /**
     * @brief 等待重试延迟
     * @param delay 延迟时间
     */
    void WaitForRetryDelay(std::chrono::milliseconds delay);
    
    /**
     * @brief 构建错误上下文信息
     * @param error 错误信息
     * @return 上下文字符串
     */
    std::wstring BuildErrorContext(const RotationError& error) const;
};

/**
 * @brief 错误处理器工厂
 */
class ErrorHandlerFactory {
public:
    /**
     * @brief 创建标准错误处理器
     * @return 错误处理器实例
     */
    static std::unique_ptr<RotationErrorHandler> CreateStandard();
    
    /**
     * @brief 创建快速失败错误处理器（不重试）
     * @return 错误处理器实例
     */
    static std::unique_ptr<RotationErrorHandler> CreateFailFast();
    
    /**
     * @brief 创建高容错错误处理器（更多重试）
     * @return 错误处理器实例
     */
    static std::unique_ptr<RotationErrorHandler> CreateResilient();
    
    /**
     * @brief 创建自定义配置错误处理器
     * @param config 重试配置
     * @return 错误处理器实例
     */
    static std::unique_ptr<RotationErrorHandler> CreateCustom(const RetryConfig& config);
};

/**
 * @brief 错误处理工具类
 */
class ErrorHandlerUtils {
public:
    /**
     * @brief 从异常创建轮转错误
     * @param ex 异常对象
     * @param operation 操作名称
     * @return 轮转错误
     */
    static RotationError CreateErrorFromException(const std::exception& ex,
                                                 const std::wstring& operation = L"");
    
    /**
     * @brief 从系统错误码创建轮转错误
     * @param errorCode 系统错误码
     * @param operation 操作名称
     * @return 轮转错误
     */
    static RotationError CreateErrorFromSystemCode(int errorCode,
                                                   const std::wstring& operation = L"");
    
    /**
     * @brief 格式化错误消息
     * @param error 错误信息
     * @return 格式化后的消息
     */
    static std::wstring FormatErrorMessage(const RotationError& error);
    
    /**
     * @brief 格式化恢复结果
     * @param result 恢复结果
     * @return 格式化后的消息
     */
    static std::wstring FormatRecoveryResult(const RecoveryResult& result);
};