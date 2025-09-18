#define _CRT_SECURE_NO_WARNINGS

#include "log/AsyncRotationManager.h"
#include "log/RotationStrategies.h"
#include "log/LogCompressor.h"
#include <algorithm>
#include <sstream>
#include <random>
#include <iomanip>
#include <ctime>
#include <cstring>
#include <filesystem>

/*****************************************************************************
 *  AsyncRotationManager
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.        UpdateStatistics(result);
        
        processingTaskCount_--;
        RemoveTaskInfo(request.requestId);
        
        return result;
    }
    
    processingTaskCount_--;
    RemoveTaskInfo(request.requestId);
    
    // 返回默认失败结果
    RotationResult failResult;
    failResult.success = false;
    failResult.errorMessage = "Unknown error";
    return failResult;ile     AsyncRotationManager.cpp
 *  @brief    异步轮转管理器实现
 *  @details  提供非阻塞轮转操作和任务队列管理，集成所有轮转组件
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/16
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

AsyncRotationManager::AsyncRotationManager(const AsyncRotationConfig& config)
    : asyncConfig_(config),
      transactionManager_(std::make_unique<TransactionalRotationManager>()),
      preChecker_(std::make_unique<RotationPreChecker>()),
      compressor_(std::make_shared<LogCompressor>()) {
    
    // 设置默认轮转策略
    rotationStrategy_ = std::make_shared<SizeBasedRotationStrategy>(50 * 1024 * 1024); // 50MB
}

AsyncRotationManager::~AsyncRotationManager() {
    Stop();
}

// ILogRotationManager接口实现
void AsyncRotationManager::SetConfig(const LogRotationConfig& config) {
    std::lock_guard<std::mutex> lock(configMutex_);
    rotationConfig_ = config;
}

LogRotationConfig AsyncRotationManager::GetConfig() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return rotationConfig_;
}

RotationTrigger AsyncRotationManager::CheckRotationNeeded(const std::wstring& currentFileName, 
                                                          size_t fileSize) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    if (!rotationStrategy_) {
        return RotationTrigger{};
    }
    
    // 创建轮转上下文
    RotationContext context;
    context.currentFileName = currentFileName;
    context.currentFileSize = fileSize;
    context.currentTime = std::chrono::system_clock::now();
    context.lastRotationTime = statistics_.lastRotationTime;
    context.manualTrigger = false;
    
    // 调用策略判断
    RotationDecision decision = rotationStrategy_->ShouldRotate(context);
    
    RotationTrigger trigger;
    if (decision.shouldRotate) {
        // 根据轮转策略判断触发类型
        if (fileSize >= rotationConfig_.maxFileSizeMB * 1024 * 1024) {
            trigger.sizeExceeded = true;
        }
        // 检查时间条件
        auto now = std::chrono::system_clock::now();
        auto timeSinceLastRotation = now - statistics_.lastRotationTime;
        if (rotationConfig_.strategy == LogRotationStrategy::Time || 
            rotationConfig_.strategy == LogRotationStrategy::SizeAndTime) {
            switch (rotationConfig_.timeInterval) {
                case TimeRotationInterval::Hourly:
                    if (timeSinceLastRotation >= std::chrono::hours(1)) {
                        trigger.timeReached = true;
                    }
                    break;
                case TimeRotationInterval::Daily:
                    if (timeSinceLastRotation >= std::chrono::hours(24)) {
                        trigger.timeReached = true;
                    }
                    break;
                case TimeRotationInterval::Weekly:
                    if (timeSinceLastRotation >= std::chrono::hours(24 * 7)) {
                        trigger.timeReached = true;
                    }
                    break;
                case TimeRotationInterval::Monthly:
                    if (timeSinceLastRotation >= std::chrono::hours(24 * 30)) {
                        trigger.timeReached = true;
                    }
                    break;
            }
        }
        
        trigger.currentFileSize = fileSize;
        trigger.reason = decision.reason;
    }
    
    return trigger;
}

RotationResult AsyncRotationManager::PerformRotation(const std::wstring& currentFileName,
                                                     const RotationTrigger& trigger) {
    // 创建异步请求
    AsyncRotationRequest request(GenerateRequestId(), currentFileName);
    request.context.currentFileName = currentFileName;
    request.context.currentFileSize = trigger.currentFileSize;
    request.context.currentTime = std::chrono::system_clock::now();
    request.context.lastRotationTime = statistics_.lastRotationTime;
    request.context.manualTrigger = trigger.manualRequested;
    
    return ProcessRotationRequest(request);
}

RotationResult AsyncRotationManager::ForceRotation(const std::wstring& currentFileName,
                                                   const std::wstring& reason) {
    RotationTrigger trigger;
    trigger.reason = reason;
    trigger.manualRequested = true;
    trigger.currentFileSize = 0; // 手动轮转时大小不重要
    
    return PerformRotation(currentFileName, trigger);
}

void AsyncRotationManager::SetRotationCallback(RotationCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    rotationCallback_ = callback;
}

RotationStatistics AsyncRotationManager::GetStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return statistics_;
}

void AsyncRotationManager::ResetStatistics() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    statistics_ = RotationStatistics{};
}

size_t AsyncRotationManager::CleanupOldArchives() {
    if (!rotationStrategy_) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(configMutex_);
    
    // 实现清理逻辑
    size_t cleanedCount = 0;
    try {
        if (!rotationConfig_.archiveDirectory.empty()) {
            std::filesystem::path archiveDir = std::filesystem::path(rotationConfig_.archiveDirectory);
            if (std::filesystem::exists(archiveDir)) {
                // 计算文件保留时间（基于最大文件数）
                size_t maxFiles = rotationConfig_.maxArchiveFiles;
                std::vector<std::filesystem::directory_entry> archiveFiles;
                
                for (const auto& entry : std::filesystem::directory_iterator(archiveDir)) {
                    if (entry.is_regular_file()) {
                        archiveFiles.push_back(entry);
                    }
                }
                
                if (archiveFiles.size() > maxFiles) {
                    // 按修改时间排序
                    std::sort(archiveFiles.begin(), archiveFiles.end(),
                        [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
                            return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
                        });
                    
                    // 删除多余的文件
                    for (size_t i = 0; i < archiveFiles.size() - maxFiles; ++i) {
                        std::filesystem::remove(archiveFiles[i]);
                        cleanedCount++;
                    }
                }
            }
        }
    } catch (const std::exception&) {
        // 忽略清理错误
    }
    
    return cleanedCount;
}

bool AsyncRotationManager::GetNextRotationTime(std::chrono::system_clock::time_point& nextTime) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    if (!rotationStrategy_) {
        return false;
    }
    
    // 实现获取下次轮转时间的逻辑
    nextTime = std::chrono::system_clock::now() + std::chrono::hours(1); // 默认1小时后
    return true;
}

void AsyncRotationManager::Start() {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    if (isRunning_.load()) {
        return;
    }
    
    stopRequested_ = false;
    StartWorkerThreads();
    isRunning_ = true;
}

void AsyncRotationManager::Stop() {
    std::unique_lock<std::mutex> lock(configMutex_);
    
    if (!isRunning_.load()) {
        return;
    }
    
    stopRequested_ = true;
    lock.unlock();
    
    // 通知所有工作线程停止
    queueCondVar_.notify_all();
    
    // 等待所有工作线程结束
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    workerThreads_.clear();
    isRunning_ = false;
}

bool AsyncRotationManager::IsRunning() const {
    return isRunning_.load();
}

// 异步扩展功能实现
std::future<RotationResult> AsyncRotationManager::PerformRotationAsync(const std::wstring& currentFileName,
                                                                        const RotationTrigger& trigger) {
    AsyncRotationRequest request(GenerateRequestId(), currentFileName);
    request.context.currentFileName = currentFileName;
    request.context.currentFileSize = trigger.currentFileSize;
    request.context.currentTime = std::chrono::system_clock::now();
    request.context.lastRotationTime = statistics_.lastRotationTime;
    request.context.manualTrigger = trigger.manualRequested;
    
    auto future = request.promise.get_future();
    
    // 加入队列
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (requestQueue_.size() >= asyncConfig_.maxQueueSize) {
            RotationResult result;
            result.success = false;
            result.errorMessage = "Queue is full";
            request.promise.set_value(result);
            return future;
        }
        requestQueue_.push(std::move(request));
    }
    
    queueCondVar_.notify_one();
    return future;
}

std::future<RotationResult> AsyncRotationManager::ForceRotationAsync(const std::wstring& currentFileName,
                                                                      const std::wstring& reason) {
    RotationTrigger trigger;
    trigger.reason = reason;
    trigger.manualRequested = true;
    
    return PerformRotationAsync(currentFileName, trigger);
}

bool AsyncRotationManager::CancelRotationRequest(const std::wstring& requestId) {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    auto it = activeTasks_.find(requestId);
    if (it != activeTasks_.end()) {
        auto& taskInfo = it->second;
        if (taskInfo->status == RotationTaskStatus::Pending) {
            taskInfo->status = RotationTaskStatus::Cancelled;
            taskInfo->endTime = std::chrono::system_clock::now();
            return true;
        }
    }
    
    return false;
}

size_t AsyncRotationManager::GetPendingRequestCount() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return requestQueue_.size();
}

size_t AsyncRotationManager::GetProcessingTaskCount() const {
    return processingTaskCount_.load();
}

size_t AsyncRotationManager::GetPendingTaskCount() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return requestQueue_.size();
}

std::shared_ptr<RotationTaskInfo> AsyncRotationManager::GetTaskInfo(const std::wstring& taskId) const {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    auto it = activeTasks_.find(taskId);
    if (it != activeTasks_.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::vector<RotationTaskInfo> AsyncRotationManager::GetAllTaskInfo() const {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    std::vector<RotationTaskInfo> taskList;
    for (const auto& pair : activeTasks_) {
        taskList.push_back(*pair.second);
    }
    
    return taskList;
}

void AsyncRotationManager::SetRotationStrategy(RotationStrategySharedPtr strategy) {
    std::lock_guard<std::mutex> lock(configMutex_);
    rotationStrategy_ = strategy;
}

RotationStrategySharedPtr AsyncRotationManager::GetRotationStrategy() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return rotationStrategy_;
}

void AsyncRotationManager::SetPreChecker(std::unique_ptr<RotationPreChecker> preChecker) {
    std::lock_guard<std::mutex> lock(configMutex_);
    preChecker_ = std::move(preChecker);
}

RotationPreChecker* AsyncRotationManager::GetPreChecker() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return preChecker_.get();
}

void AsyncRotationManager::SetCompressor(std::shared_ptr<ILogCompressor> compressor) {
    std::lock_guard<std::mutex> lock(configMutex_);
    compressor_ = compressor;
}

std::shared_ptr<ILogCompressor> AsyncRotationManager::GetCompressor() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return compressor_;
}

void AsyncRotationManager::SetAsyncConfig(const AsyncRotationConfig& config) {
    std::lock_guard<std::mutex> lock(configMutex_);
    asyncConfig_ = config;
}

AsyncRotationConfig AsyncRotationManager::GetAsyncConfig() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return asyncConfig_;
}

std::wstring AsyncRotationManager::GetManagerStatus() const {
    std::wostringstream oss;
    oss << L"AsyncRotationManager Status:\n";
    oss << L"  Running: " << (IsRunning() ? L"Yes" : L"No") << L"\n";
    oss << L"  Pending Requests: " << GetPendingRequestCount() << L"\n";
    oss << L"  Processing Tasks: " << GetProcessingTaskCount() << L"\n";
    oss << L"  Worker Threads: " << workerThreads_.size() << L"\n";
    
    return oss.str();
}

// 私有方法实现
void AsyncRotationManager::StartWorkerThreads() {
    for (size_t i = 0; i < asyncConfig_.workerThreadCount; ++i) {
        workerThreads_.emplace_back(&AsyncRotationManager::WorkerThreadLoop, this, i);
    }
}

void AsyncRotationManager::StopWorkerThreads() {
    stopRequested_ = true;
    queueCondVar_.notify_all();
    
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    workerThreads_.clear();
}

void AsyncRotationManager::WorkerThreadLoop(size_t threadId) {
    while (!stopRequested_.load()) {
        AsyncRotationRequest request;
        
        // 从队列获取请求
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCondVar_.wait(lock, [this] { 
                return stopRequested_.load() || !requestQueue_.empty(); 
            });
            
            if (stopRequested_.load()) {
                break;
            }
            
            if (!requestQueue_.empty()) {
                request = std::move(const_cast<AsyncRotationRequest&>(requestQueue_.top()));
                requestQueue_.pop();
            } else {
                continue;
            }
        }
        
        // 处理请求
        try {
            RotationResult result = ProcessRotationRequest(request);
            request.promise.set_value(result);
        } catch (const std::exception& ex) {
            RotationResult result;
            result.success = false;
            std::string temp(ex.what());
            result.errorMessage = "Exception: " + temp;
            request.promise.set_value(result);
        }
    }
}

RotationResult AsyncRotationManager::ProcessRotationRequest(AsyncRotationRequest& request) {
    // 创建任务信息
    auto taskInfo = std::make_shared<RotationTaskInfo>(request.requestId, request.currentFileName);
    taskInfo->priority = request.priority;
    taskInfo->status = RotationTaskStatus::Processing;
    
    AddTaskInfo(taskInfo);
    processingTaskCount_++;
    // 执行轮转任务
    RotationResult result = ExecuteRotationTask(taskInfo, request.context);
    try {
        
        
        // 更新任务状态
        if (result.success) {
            UpdateTaskStatus(request.requestId, RotationTaskStatus::Completed);
        } else {
            // 将 std::string 转换为 std::wstring
            std::string errMsg = result.errorMessage;
            std::wstring wErrMsg(errMsg.begin(), errMsg.end());
            UpdateTaskStatus(request.requestId, RotationTaskStatus::Failed, wErrMsg);
        }
        
        // 更新统计信息
        UpdateStatistics(result);
        
        // 触发回调
        TriggerRotationCallback(result);
        
        return result;
        
    } catch (const std::exception& ex) {
        std::string tempMsg = std::string("Process rotation request exception: ") + ex.what();
        std::wstring errorMsg(tempMsg.begin(), tempMsg.end());
        
        UpdateTaskStatus(request.requestId, RotationTaskStatus::Failed, errorMsg);
        
        RotationResult result;
        result.success = false;
        result.errorMessage = tempMsg;
        
        UpdateStatistics(result);
        TriggerRotationCallback(result);
        
        processingTaskCount_--;
        RemoveTaskInfo(request.requestId);
        
        return result;
    }
    
    processingTaskCount_--;
    RemoveTaskInfo(request.requestId);
    
    return result;
}

RotationResult AsyncRotationManager::ExecuteRotationTask(std::shared_ptr<RotationTaskInfo> taskInfo,
                                                         const RotationContext& context) {
    RotationResult result;
    result.success = false;
    
    try {
        // 预检查
        if (asyncConfig_.enablePreCheck && preChecker_) {
            RotationCheckContext checkContext;
            checkContext.sourceFile = context.currentFileName;
            checkContext.archiveDirectory = rotationConfig_.archiveDirectory;
            
            PreCheckResult checkResult = preChecker_->CheckRotationConditions(checkContext);
            if (!checkResult.canRotate) {
                std::string errMsg = "Precheck error";
                if (!checkResult.results.empty()) {
                    std::wstring wMsg = checkResult.results[0].message;
                    errMsg = std::string(wMsg.begin(), wMsg.end());
                }
                result.errorMessage = errMsg;
                return result;
            }
        }
        
        // 执行事务性轮转
        if (asyncConfig_.enableTransaction && transactionManager_) {
            std::wstring archiveFileName = GenerateArchiveFileName(context.currentFileName);
            auto transaction = transactionManager_->CreateTransaction(L"Rotation-" + taskInfo->taskId);
            
            // 添加文件移动操作
            transaction->AddFileMoveOperation(context.currentFileName, archiveFileName);
            
            // 如果启用压缩
            if (rotationConfig_.enableCompression && compressor_) {
                std::wstring compressedFileName = archiveFileName + L".zip";
                transaction->AddCompressionOperation(archiveFileName, compressedFileName);
            }
            
            // 执行事务
            if (transaction->Execute()) {
                result.success = true;
                result.newFileName = archiveFileName;
                result.rotationTime = std::chrono::system_clock::now();
                
                // 清理旧文件
                CleanupOldArchives();
            } else {
                result.errorMessage = "Transaction execution failed";
            }
        } else {
            // 简单文件移动
            std::wstring archiveFileName = GenerateArchiveFileName(context.currentFileName);
            
            try {
                std::filesystem::rename(context.currentFileName, archiveFileName);
                result.success = true;
                result.newFileName = archiveFileName;
                result.rotationTime = std::chrono::system_clock::now();
            } catch (const std::filesystem::filesystem_error& ex) {
                std::string errMsg = "File movement failed: " + std::string(ex.what());
                result.errorMessage = errMsg;
            }
        }
        
    } catch (const std::exception& ex) {
        std::string errMsg = "Abnormal execution of rotation tasks: " + std::string(ex.what());
        result.errorMessage = errMsg;
    }
    
    return result;
}

std::wstring AsyncRotationManager::GenerateRequestId() const {
    static std::atomic<uint64_t> counter{0};
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count() % 1000;
    
    std::wostringstream oss;
    oss << L"REQ_" << time_t << L"_" << ms << L"_" << counter++;
    
    return oss.str();
}

std::wstring AsyncRotationManager::GenerateArchiveFileName(const std::wstring& originalFileName) const {
    std::filesystem::path originalPath(originalFileName);
    std::wstring baseName = originalPath.stem().wstring();
    std::wstring extension = originalPath.extension().wstring();
    
    std::time_t now = std::time(nullptr);
    std::tm* timeInfo = std::localtime(&now);
    
    std::wostringstream oss;
    
    // 需要锁定配置访问
    std::lock_guard<std::mutex> lock(configMutex_);
    oss << rotationConfig_.archiveDirectory << L"/" << baseName
        << L"_" << std::put_time(timeInfo, L"%Y%m%d_%H%M%S")
        << extension;
    
    return oss.str();
}

void AsyncRotationManager::AddTaskInfo(std::shared_ptr<RotationTaskInfo> taskInfo) {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    activeTasks_[taskInfo->taskId] = taskInfo;
}

void AsyncRotationManager::RemoveTaskInfo(const std::wstring& taskId) {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    activeTasks_.erase(taskId);
}

void AsyncRotationManager::UpdateTaskStatus(const std::wstring& taskId, RotationTaskStatus status,
                                            const std::wstring& errorMessage) {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    auto it = activeTasks_.find(taskId);
    if (it != activeTasks_.end()) {
        it->second->status = status;
        it->second->endTime = std::chrono::system_clock::now();
        if (!errorMessage.empty()) {
            it->second->errorMessage = errorMessage;
        }
    }
}

void AsyncRotationManager::TriggerRotationCallback(const RotationResult& result) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    
    if (rotationCallback_) {
        try {
            rotationCallback_(result);
        } catch (...) {
            // 忽略回调异常
        }
    }
}

void AsyncRotationManager::UpdateStatistics(const RotationResult& result) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    statistics_.totalRotations++;
    if (result.success) {
        statistics_.successfulRotations++;
    } else {
        statistics_.failedRotations++;
    }
    statistics_.lastRotationTime = std::chrono::system_clock::now();
}

size_t AsyncRotationManager::CleanupExpiredTasks() {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    size_t cleanedCount = 0;
    auto now = std::chrono::system_clock::now();
    
    for (auto it = activeTasks_.begin(); it != activeTasks_.end();) {
        const auto& taskInfo = it->second;
        if (taskInfo->status == RotationTaskStatus::Completed || 
            taskInfo->status == RotationTaskStatus::Failed) {
            
            auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(
                now - taskInfo->endTime);
            
            if (elapsed > std::chrono::minutes(60)) { // 1小时后清理完成的任务
                it = activeTasks_.erase(it);
                cleanedCount++;
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
    
    return cleanedCount;
}

bool AsyncRotationManager::WaitForQueueSpace(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(queueMutex_);
    
    return queueCondVar_.wait_for(lock, timeout, [this] {
        return requestQueue_.size() < asyncConfig_.maxQueueSize;
    });
}

bool AsyncRotationManager::ValidateRotationRequest(const AsyncRotationRequest& request) const {
    if (request.requestId.empty() || request.currentFileName.empty()) {
        return false;
    }
    
    if (!std::filesystem::exists(request.currentFileName)) {
        return false;
    }
    
    return true;
}

// 工厂类实现
std::unique_ptr<AsyncRotationManager> AsyncRotationManagerFactory::CreateStandard(
    const AsyncRotationConfig& config) {
    
    return std::make_unique<AsyncRotationManager>(config);
}

std::unique_ptr<AsyncRotationManager> AsyncRotationManagerFactory::CreateHighPerformance(
    const AsyncRotationConfig& config) {
    
    AsyncRotationConfig highPerfConfig = config;
    highPerfConfig.workerThreadCount = std::max(4u, std::thread::hardware_concurrency());
    highPerfConfig.maxQueueSize = 5000;
    highPerfConfig.taskTimeout = std::chrono::milliseconds(30000);
    
    return std::make_unique<AsyncRotationManager>(highPerfConfig);
}

std::unique_ptr<AsyncRotationManager> AsyncRotationManagerFactory::CreateComplete(
    const LogRotationConfig& rotationConfig,
    const AsyncRotationConfig& asyncConfig,
    RotationStrategySharedPtr strategy,
    std::shared_ptr<ILogCompressor> compressor) {
    
    auto manager = std::make_unique<AsyncRotationManager>(asyncConfig);
    manager->SetConfig(rotationConfig);
    manager->SetRotationStrategy(strategy);
    manager->SetCompressor(compressor);
    
    return manager;
}

// 实现 ILogRotationManager 接口中的缺失方法
size_t AsyncRotationManager::GetActiveTaskCount() const {
    return GetProcessingTaskCount();
}

size_t AsyncRotationManager::CancelPendingTasks() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    size_t cancelledCount = requestQueue_.size();
    
    // 清空队列中的所有请求
    while (!requestQueue_.empty()) {
        AsyncRotationRequest request = std::move(const_cast<AsyncRotationRequest&>(requestQueue_.top()));
        requestQueue_.pop();
        
        // 设置取消结果
        RotationResult result;
        result.success = false;
        result.errorMessage = "Task cancelled";
        request.promise.set_value(result);
    }
    
    return cancelledCount;
}

bool AsyncRotationManager::WaitForAllTasks(std::chrono::milliseconds timeout) {
    auto startTime = std::chrono::steady_clock::now();
    
    while (true) {
        if (GetPendingRequestCount() == 0 && GetProcessingTaskCount() == 0) {
            return true;
        }
        
        if (timeout.count() > 0) {
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed >= timeout) {
                return false;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}