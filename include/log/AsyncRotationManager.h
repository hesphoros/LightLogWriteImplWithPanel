#pragma once

#include "ILogRotationManager.h"
#include "IRotationStrategy.h"
#include "RotationStateMachine.h"
#include "TransactionalRotation.h"
#include "RotationPreChecker.h"
#include "ILogCompressor.h"
#include <queue>
#include <thread>
#include <future>
#include <condition_variable>
#include <atomic>
#include <mutex>

/*****************************************************************************
 *  AsyncRotationManager
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     AsyncRotationManager.h
 *  @brief    异步轮转管理器
 *  @details  提供非阻塞轮转操作和任务队列管理，集成所有轮转组件
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/15
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

/**
 * @brief 异步轮转请求
 */
struct AsyncRotationRequest {
    std::wstring requestId;                             /*!< 请求ID */
    std::wstring currentFileName;                       /*!< 当前文件名 */
    RotationContext context;                            /*!< 轮转上下文 */
    std::promise<RotationResult> promise;               /*!< 结果承诺 */
    std::chrono::system_clock::time_point requestTime; /*!< 请求时间 */
    int priority = 0;                                   /*!< 优先级 */
    bool manualTrigger = false;                         /*!< 是否手动触发 */
    
    AsyncRotationRequest() : requestTime(std::chrono::system_clock::now()) {}
    
    AsyncRotationRequest(const std::wstring& id, const std::wstring& fileName)
        : requestId(id), currentFileName(fileName), requestTime(std::chrono::system_clock::now()) {}
};

/**
 * @brief 轮转请求比较器（优先级队列用）
 */
struct RotationRequestComparator {
    bool operator()(const AsyncRotationRequest& a, const AsyncRotationRequest& b) const {
        // 优先级高的排在前面，时间早的排在前面
        if (a.priority != b.priority) {
            return a.priority < b.priority;
        }
        return a.requestTime > b.requestTime;
    }
};

/**
 * @brief 轮转任务状态
 */
enum class RotationTaskStatus {
    Pending,        /*!< 等待处理 */
    Processing,     /*!< 正在处理 */
    Completed,      /*!< 已完成 */
    Failed,         /*!< 失败 */
    Cancelled       /*!< 已取消 */
};

/**
 * @brief 轮转任务信息
 */
struct RotationTaskInfo {
    std::wstring taskId;                                /*!< 任务ID */
    std::wstring fileName;                              /*!< 文件名 */
    RotationTaskStatus status = RotationTaskStatus::Pending; /*!< 任务状态 */
    std::chrono::system_clock::time_point startTime;   /*!< 开始时间 */
    std::chrono::system_clock::time_point endTime;     /*!< 结束时间 */
    std::wstring errorMessage;                          /*!< 错误消息 */
    int priority = 0;                                   /*!< 优先级 */
    
    RotationTaskInfo() : startTime(std::chrono::system_clock::now()) {}
    
    RotationTaskInfo(const std::wstring& id, const std::wstring& file)
        : taskId(id), fileName(file), startTime(std::chrono::system_clock::now()) {}
    
    /**
     * @brief 获取任务执行时间
     * @return 执行时间
     */
    std::chrono::milliseconds GetExecutionTime() const {
        if (status == RotationTaskStatus::Processing) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - startTime);
        } else if (status == RotationTaskStatus::Completed || status == RotationTaskStatus::Failed) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        }
        return std::chrono::milliseconds(0);
    }
};

/**
 * @brief 异步轮转管理器配置
 */
struct AsyncRotationConfig {
    size_t maxQueueSize = 1000;                        /*!< 最大队列大小 */
    size_t workerThreadCount = 2;                      /*!< 工作线程数量 */
    std::chrono::milliseconds taskTimeout{60000};     /*!< 任务超时时间 */
    std::chrono::milliseconds queueTimeout{5000};     /*!< 队列等待超时 */
    bool enablePreCheck = true;                        /*!< 启用预检查 */
    bool enableTransaction = true;                     /*!< 启用事务机制 */
    bool enableStateMachine = true;                    /*!< 启用状态机 */
    size_t maxRetryCount = 3;                          /*!< 最大重试次数 */
    std::chrono::milliseconds retryDelay{1000};       /*!< 重试延迟 */
    
    AsyncRotationConfig() = default;
};

/**
 * @brief 异步轮转管理器
 * @details 提供完整的异步轮转功能，集成策略、状态机、事务、预检查等组件
 */
class AsyncRotationManager : public ILogRotationManager {
public:
    /**
     * @brief 构造函数
     * @param config 配置
     */
    explicit AsyncRotationManager(const AsyncRotationConfig& config = AsyncRotationConfig{});
    
    /**
     * @brief 析构函数
     */
    virtual ~AsyncRotationManager();
    
    // ILogRotationManager接口实现
    void SetConfig(const LogRotationConfig& config) override;
    LogRotationConfig GetConfig() const override;
    
    RotationTrigger CheckRotationNeeded(const std::wstring& currentFileName, 
                                       size_t fileSize) const override;
    
    RotationResult PerformRotation(const std::wstring& currentFileName,
                                  const RotationTrigger& trigger) override;
    
    RotationResult ForceRotation(const std::wstring& currentFileName,
                                const std::wstring& reason = L"Manual rotation") override;
    
    void SetRotationCallback(RotationCallback callback) override;
    
    RotationStatistics GetStatistics() const override;
    void ResetStatistics() override;
    
    size_t CleanupOldArchives() override;
    bool GetNextRotationTime(std::chrono::system_clock::time_point& nextTime) const override;
    
    void Start() override;
    void Stop() override;
    bool IsRunning() const override;
    
    // ILogRotationManager 接口中的其他方法
    std::future<RotationResult> PerformRotationAsync(const std::wstring& currentFileName,
                                                     const RotationTrigger& trigger) override;
    size_t GetPendingTaskCount() const override;
    size_t GetActiveTaskCount() const override;
    size_t CancelPendingTasks() override;
    bool WaitForAllTasks(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) override;
    
    // 异步扩展功能
    
    /**
     * @brief 异步强制轮转
     * @param currentFileName 当前文件名
     * @param reason 轮转原因
     * @return 异步结果
     */
    std::future<RotationResult> ForceRotationAsync(const std::wstring& currentFileName,
                                                   const std::wstring& reason = L"Manual rotation");
    
    /**
     * @brief 取消轮转请求
     * @param requestId 请求ID
     * @return 是否取消成功
     */
    bool CancelRotationRequest(const std::wstring& requestId);
    
    /**
     * @brief 获取等待队列大小
     * @return 队列大小
     */
    size_t GetPendingRequestCount() const;
    
    /**
     * @brief 获取正在处理的任务数量
     * @return 处理中任务数量
     */
    size_t GetProcessingTaskCount() const;
    
    /**
     * @brief 获取任务信息
     * @param taskId 任务ID
     * @return 任务信息，如果不存在返回nullptr
     */
    std::shared_ptr<RotationTaskInfo> GetTaskInfo(const std::wstring& taskId) const;
    
    /**
     * @brief 获取所有任务信息
     * @return 任务信息列表
     */
    std::vector<RotationTaskInfo> GetAllTaskInfo() const;
    
    /**
     * @brief 设置轮转策略
     * @param strategy 轮转策略
     */
    void SetRotationStrategy(RotationStrategySharedPtr strategy);
    
    /**
     * @brief 获取轮转策略
     * @return 轮转策略
     */
    RotationStrategySharedPtr GetRotationStrategy() const;
    
    /**
     * @brief 设置预检查器
     * @param preChecker 预检查器
     */
    void SetPreChecker(std::unique_ptr<RotationPreChecker> preChecker);
    
    /**
     * @brief 获取预检查器
     * @return 预检查器
     */
    RotationPreChecker* GetPreChecker() const;
    
    /**
     * @brief 设置压缩器
     * @param compressor 压缩器
     */
    void SetCompressor(std::shared_ptr<ILogCompressor> compressor);
    
    /**
     * @brief 获取压缩器
     * @return 压缩器
     */
    std::shared_ptr<ILogCompressor> GetCompressor() const;
    
    /**
     * @brief 设置异步配置
     * @param config 异步配置
     */
    void SetAsyncConfig(const AsyncRotationConfig& config);
    
    /**
     * @brief 获取异步配置
     * @return 异步配置
     */
    AsyncRotationConfig GetAsyncConfig() const;
    
    /**
     * @brief 获取管理器状态信息
     * @return 状态信息字符串
     */
    std::wstring GetManagerStatus() const;

private:
    // 配置
    mutable std::mutex configMutex_;
    LogRotationConfig rotationConfig_;
    AsyncRotationConfig asyncConfig_;
    
    // 组件
    RotationStrategySharedPtr rotationStrategy_;
    std::unique_ptr<RotationPreChecker> preChecker_;
    std::shared_ptr<ILogCompressor> compressor_;
    std::unique_ptr<TransactionalRotationManager> transactionManager_;
    
    // 任务队列
    mutable std::mutex queueMutex_;
    std::priority_queue<AsyncRotationRequest, std::vector<AsyncRotationRequest>, 
                       RotationRequestComparator> requestQueue_;
    std::condition_variable queueCondVar_;
    
    // 工作线程
    std::vector<std::thread> workerThreads_;
    std::atomic<bool> stopRequested_{false};
    std::atomic<bool> isRunning_{false};
    
    // 任务管理
    mutable std::mutex tasksMutex_;
    std::map<std::wstring, std::shared_ptr<RotationTaskInfo>> activeTasks_;
    std::atomic<size_t> processingTaskCount_{0};
    
    // 回调管理
    mutable std::mutex callbackMutex_;
    RotationCallback rotationCallback_;
    
    // 统计信息
    mutable std::mutex statsMutex_;
    RotationStatistics statistics_;
    
    // 内部方法
    
    /**
     * @brief 启动工作线程
     */
    void StartWorkerThreads();
    
    /**
     * @brief 停止工作线程
     */
    void StopWorkerThreads();
    
    /**
     * @brief 工作线程主循环
     * @param threadId 线程ID
     */
    void WorkerThreadLoop(size_t threadId);
    
    /**
     * @brief 处理轮转请求
     * @param request 轮转请求
     * @return 轮转结果
     */
    RotationResult ProcessRotationRequest(AsyncRotationRequest& request);
    
    /**
     * @brief 执行轮转任务
     * @param taskInfo 任务信息
     * @param context 轮转上下文
     * @return 轮转结果
     */
    RotationResult ExecuteRotationTask(std::shared_ptr<RotationTaskInfo> taskInfo,
                                      const RotationContext& context);
    
    /**
     * @brief 生成唯一请求ID
     * @return 请求ID
     */
    std::wstring GenerateRequestId() const;
    
    /**
     * @brief 添加任务信息
     * @param taskInfo 任务信息
     */
    void AddTaskInfo(std::shared_ptr<RotationTaskInfo> taskInfo);
    
    /**
     * @brief 移除任务信息
     * @param taskId 任务ID
     */
    void RemoveTaskInfo(const std::wstring& taskId);
    
    /**
     * @brief 更新任务状态
     * @param taskId 任务ID
     * @param status 新状态
     * @param errorMessage 错误消息（可选）
     */
    void UpdateTaskStatus(const std::wstring& taskId, RotationTaskStatus status,
                         const std::wstring& errorMessage = L"");
    
    /**
     * @brief 触发轮转回调
     * @param result 轮转结果
     */
    void TriggerRotationCallback(const RotationResult& result);
    
    /**
     * @brief 更新统计信息
     * @param result 轮转结果
     */
    void UpdateStatistics(const RotationResult& result);
    
    /**
     * @brief 清理过期任务
     * @return 清理的任务数量
     */
    size_t CleanupExpiredTasks();
    
    /**
     * @brief 等待队列有空间
     * @param timeout 超时时间
     * @return 是否有空间
     */
    bool WaitForQueueSpace(std::chrono::milliseconds timeout);
    
    /**
     * @brief 验证轮转请求
     * @param request 轮转请求
     * @return 验证是否通过
     */
    bool ValidateRotationRequest(const AsyncRotationRequest& request) const;
    
    /**
     * @brief 生成归档文件名
     * @param originalFileName 原始文件名
     * @return 归档文件名
     */
    std::wstring GenerateArchiveFileName(const std::wstring& originalFileName) const;
};

/**
 * @brief 异步轮转管理器工厂
 */
class AsyncRotationManagerFactory {
public:
    /**
     * @brief 创建标准异步轮转管理器
     * @param config 异步配置
     * @return 管理器实例
     */
    static std::unique_ptr<AsyncRotationManager> CreateStandard(
        const AsyncRotationConfig& config = AsyncRotationConfig{});
    
    /**
     * @brief 创建高性能异步轮转管理器
     * @param config 异步配置
     * @return 管理器实例
     */
    static std::unique_ptr<AsyncRotationManager> CreateHighPerformance(
        const AsyncRotationConfig& config = AsyncRotationConfig{});
    
    /**
     * @brief 创建完整功能异步轮转管理器
     * @param rotationConfig 轮转配置
     * @param asyncConfig 异步配置
     * @param strategy 轮转策略
     * @param compressor 压缩器
     * @return 管理器实例
     */
    static std::unique_ptr<AsyncRotationManager> CreateComplete(
        const LogRotationConfig& rotationConfig,
        const AsyncRotationConfig& asyncConfig,
        RotationStrategySharedPtr strategy,
        std::shared_ptr<ILogCompressor> compressor);
};