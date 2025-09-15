#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <memory>
#include <future>

/*****************************************************************************
 *  ILogRotationManager Interface
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     ILogRotationManager.h
 *  @brief    日志轮转管理器抽象接口
 *  @details  定义日志文件轮转功能的统一接口，支持多种轮转策略
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/15
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

/**
 * @brief 日志轮转策略枚举
 */
enum class LogRotationStrategy {
    None,           /*!< 不轮转 */
    Size,           /*!< 按文件大小轮转 */
    Time,           /*!< 按时间间隔轮转 */
    SizeAndTime     /*!< 按大小和时间轮转 */
};

/**
 * @brief 时间轮转间隔枚举
 */
enum class TimeRotationInterval {
    Hourly,         /*!< 每小时 */
    Daily,          /*!< 每天 */
    Weekly,         /*!< 每周 */
    Monthly         /*!< 每月 */
};

/**
 * @brief 轮转触发条件
 */
struct RotationTrigger {
    bool sizeExceeded = false;      /*!< 文件大小超限 */
    bool timeReached = false;       /*!< 时间间隔到达 */
    bool manualRequested = false;   /*!< 手动请求轮转 */
    size_t currentFileSize = 0;     /*!< 当前文件大小 */
    std::wstring reason;            /*!< 轮转原因描述 */
};

/**
 * @brief 轮转结果信息
 */
struct RotationResult {
    bool success = false;                                       /*!< 轮转是否成功 */
    std::wstring oldFileName;                                   /*!< 原文件名 */
    std::wstring newFileName;                                   /*!< 新文件名 */
    std::wstring archiveFileName;                               /*!< 归档文件名 */
    std::chrono::system_clock::time_point rotationTime;        /*!< 轮转时间 */
    std::chrono::milliseconds duration;                        /*!< 轮转耗时 */
    std::string errorMessage;                                   /*!< 错误信息 */
    bool compressionScheduled = false;                          /*!< 是否已安排压缩 */
    
    RotationResult() : rotationTime(std::chrono::system_clock::now()), duration(0) {}
};

/**
 * @brief 轮转配置（增强版）
 */
struct LogRotationConfig {
    LogRotationStrategy strategy = LogRotationStrategy::None;        /*!< 轮转策略 */
    size_t maxFileSizeMB = 100;                                     /*!< 最大文件大小(MB) */
    TimeRotationInterval timeInterval = TimeRotationInterval::Daily; /*!< 时间轮转间隔 */
    size_t maxArchiveFiles = 10;                                    /*!< 最大归档文件数 */
    std::wstring archiveDirectory;                                  /*!< 归档目录 */
    bool enableCompression = true;                                  /*!< 启用压缩 */
    bool deleteSourceAfterArchive = true;                          /*!< 归档后删除源文件 */
    
    // 新增配置项
    bool enableAsync = true;                                        /*!< 启用异步轮转 */
    size_t asyncWorkerCount = 2;                                    /*!< 异步工作线程数 */
    bool enablePreCheck = true;                                     /*!< 启用预检查 */
    bool enableTransaction = true;                                  /*!< 启用事务机制 */
    bool enableStateMachine = true;                                 /*!< 启用状态机 */
    size_t maxRetryCount = 3;                                      /*!< 最大重试次数 */
    std::chrono::milliseconds retryDelay{1000};                   /*!< 重试延迟 */
    std::chrono::milliseconds operationTimeout{30000};            /*!< 操作超时 */
    size_t diskSpaceThresholdMB = 1024;                           /*!< 磁盘空间阈值(MB) */
    
    LogRotationConfig() = default;
};

/**
 * @brief 轮转统计信息
 */
struct RotationStatistics {
    size_t totalRotations = 0;                                      /*!< 总轮转次数 */
    size_t successfulRotations = 0;                                 /*!< 成功轮转次数 */
    size_t failedRotations = 0;                                     /*!< 失败轮转次数 */
    size_t manualRotations = 0;                                     /*!< 手动轮转次数 */
    size_t sizeTriggeredRotations = 0;                              /*!< 大小触发轮转次数 */
    size_t timeTriggeredRotations = 0;                              /*!< 时间触发轮转次数 */
    
    std::chrono::system_clock::time_point lastRotationTime;        /*!< 上次轮转时间 */
    std::chrono::milliseconds totalRotationTime{0};                /*!< 总轮转时间 */
    std::chrono::milliseconds averageRotationTime{0};              /*!< 平均轮转时间 */
    
    size_t totalArchivedFiles = 0;                                  /*!< 总归档文件数 */
    size_t totalArchivedSizeMB = 0;                                 /*!< 总归档大小(MB) */
    
    /**
     * @brief 计算成功率
     */
    double GetSuccessRate() const {
        return totalRotations > 0 ? static_cast<double>(successfulRotations) / totalRotations : 0.0;
    }
    
    RotationStatistics() : lastRotationTime(std::chrono::system_clock::now()) {}
};

/**
 * @brief 轮转事件回调函数类型
 */
using RotationCallback = std::function<void(const RotationResult&)>;

/**
 * @brief 日志轮转管理器抽象接口
 */
class ILogRotationManager {
public:
    virtual ~ILogRotationManager() = default;
    
    /**
     * @brief 设置轮转配置
     * @param config 轮转配置
     */
    virtual void SetConfig(const LogRotationConfig& config) = 0;
    
    /**
     * @brief 获取轮转配置
     * @return 当前轮转配置
     */
    virtual LogRotationConfig GetConfig() const = 0;
    
    /**
     * @brief 检查是否需要轮转
     * @param currentFileName 当前文件名
     * @param fileSize 当前文件大小
     * @return 轮转触发条件
     */
    virtual RotationTrigger CheckRotationNeeded(const std::wstring& currentFileName, 
                                                size_t fileSize) const = 0;
    
    /**
     * @brief 执行日志轮转
     * @param currentFileName 当前文件名
     * @param trigger 轮转触发条件
     * @return 轮转结果
     */
    virtual RotationResult PerformRotation(const std::wstring& currentFileName,
                                          const RotationTrigger& trigger) = 0;

    /**
     * @brief 强制执行轮转
     * @param currentFileName 当前文件名
     * @param reason 轮转原因
     * @return 轮转结果
     */
    virtual RotationResult ForceRotation(const std::wstring& currentFileName,
                                        const std::wstring& reason = L"Manual rotation") = 0;

    /**
     * @brief 设置轮转回调
     * @param callback 轮转完成时的回调函数
     */
    virtual void SetRotationCallback(RotationCallback callback) = 0;

    /**
     * @brief 获取统计信息
     * @return 轮转统计信息
     */
    virtual RotationStatistics GetStatistics() const = 0;
    
    /**
     * @brief 重置统计信息
     */
    virtual void ResetStatistics() = 0;
    
    /**
     * @brief 清理旧归档文件
     * @return 清理的文件数量
     */
    virtual size_t CleanupOldArchives() = 0;
    
    /**
     * @brief 获取下次轮转预计时间（仅时间策略有效）
     * @param nextTime 输出参数，下次轮转时间
     * @return 是否有效（时间策略才有效）
     */
    virtual bool GetNextRotationTime(std::chrono::system_clock::time_point& nextTime) const = 0;
    
    /**
     * @brief 启动轮转管理器
     */
    virtual void Start() = 0;
    
    /**
     * @brief 停止轮转管理器
     */
    virtual void Stop() = 0;
    
    /**
     * @brief 检查轮转管理器是否运行中
     */
    virtual bool IsRunning() const = 0;
    
    // 新增异步接口
    
    /**
     * @brief 异步执行轮转
     * @param currentFileName 当前文件名
     * @param trigger 轮转触发条件
     * @return 异步结果
     */
    virtual std::future<RotationResult> PerformRotationAsync(const std::wstring& currentFileName,
                                                             const RotationTrigger& trigger) = 0;
    
    /**
     * @brief 获取等待处理的轮转任务数量
     * @return 任务数量
     */
    virtual size_t GetPendingTaskCount() const = 0;
    
    /**
     * @brief 获取正在处理的轮转任务数量
     * @return 任务数量
     */
    virtual size_t GetActiveTaskCount() const = 0;
    
    /**
     * @brief 取消所有等待的轮转任务
     * @return 取消的任务数量
     */
    virtual size_t CancelPendingTasks() = 0;
    
    /**
     * @brief 等待所有轮转任务完成
     * @param timeout 超时时间，0表示无限等待
     * @return 是否在超时前完成
     */
    virtual bool WaitForAllTasks(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) = 0;
};

/**
 * @brief 智能指针类型定义
 */
using LogRotationManagerPtr = std::unique_ptr<ILogRotationManager>;

/**
 * @brief 创建默认轮转管理器的工厂函数
 * @param config 轮转配置
 * @return 轮转管理器实例
 */
LogRotationManagerPtr CreateLogRotationManager(const LogRotationConfig& config = LogRotationConfig{});