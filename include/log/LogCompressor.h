#pragma once

#define NOMINMAX
#undef min
#undef max

#include "ILogCompressor.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <BS/BS_thread_pool.hpp>

/*****************************************************************************
 *  LogCompressor
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3 as
 *  published by the Free Software Foundation.
 *
 *  @file     LogCompressor.h
 *  @brief    日志压缩器具体实现
 *  @details  基于miniz库的ZIP压缩实现，支持异步压缩和统计功能
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/15
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

 /**
  * @brief 日志压缩器配置
  */
struct LogCompressorConfig {
	size_t            maxQueueSize = 1000;                        /*!< 最大队列大小 */
	size_t       workerThreadCount = std::thread::hardware_concurrency();/*!< 工作线程数量 */
	CompressionAlgorithm algorithm = CompressionAlgorithm::ZIP; /*!< 压缩算法 */
	int           compressionLevel = 6;                          /*!< 压缩级别 (1-9, 仅ZIP有效) */
	bool deleteSourceAfterSuccess  = true;              /*!< 压缩成功后删除源文件 */
	std::chrono::milliseconds taskTimeout{ 30000 };     /*!< 单个任务超时时间 30s */
	std::chrono::milliseconds queueTimeout{ 5000 };     /*!< 队列等待超时时间 */
	bool enableStatistics          = true;                     /*!< 启用统计功能 */

	LogCompressorConfig() = default;
};


/**
 * @brief 任务队列中的内部任务表示
 */
struct InternalCompressionTask {
	CompressionTask task;                               /*!< 原始任务信息 */
	std::function<void(const CompressionResult&)> resultCallback; /*!< 详细结果回调 */
	std::chrono::system_clock::time_point startTime;   /*!< 开始处理时间 */

	InternalCompressionTask() = default;
	InternalCompressionTask(const CompressionTask& t) : task(t) {}
	InternalCompressionTask(CompressionTask&& t) : task(std::move(t)) {}
};

/**
 * @brief 优先级队列比较器
 */
struct TaskComparator {
	bool operator()(const InternalCompressionTask& a, const InternalCompressionTask& b) const {
		// 高优先级的任务排在前面，创建时间早的排在前面
		if (a.task.priority != b.task.priority) {
			return static_cast<int>(a.task.priority) < static_cast<int>(b.task.priority);
		}
		return a.task.createdTime > b.task.createdTime;
	}
};

/**
 * @brief 基于miniz的ZIP压缩器实现
 * @details 实现IStatisticalLogCompressor接口，提供完整的异步压缩功能
 */
class LogCompressor : public IStatisticalLogCompressor {
public:
	/**
	 * @brief 构造函数
	 * @param config 压缩器配置
	 */
	explicit LogCompressor(const LogCompressorConfig& config = LogCompressorConfig{});

	/**
	 * @brief 析构函数
	 */
	virtual ~LogCompressor();

	// ILogCompressor接口实现
	bool CompressFile(const std::wstring& sourceFile,
		const std::wstring& targetFile) override;

	void CompressAsync(const std::wstring& sourceFile,
		const std::wstring& targetFile,
		std::function<void(bool success)> callback = nullptr,
		CompressionPriority priority = CompressionPriority::Normal) override;

	void CompressAsyncWithResult(const std::wstring& sourceFile,
		const std::wstring& targetFile,
		std::function<void(const CompressionResult&)> callback,
		CompressionPriority priority = CompressionPriority::Normal) override;

	bool IsCompressing() const override;
	size_t GetPendingTasksCount() const override;
	size_t GetActiveTasksCount() const override;

	void Start() override;
	void Stop() override;
	bool WaitForCompletion(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) override;
	size_t CancelPendingTasks() override;

	std::vector<CompressionAlgorithm> GetSupportedAlgorithms() const override;
	bool SetCompressionAlgorithm(CompressionAlgorithm algorithm) override;
	CompressionAlgorithm GetCompressionAlgorithm() const override;

	// IStatisticalLogCompressor接口实现
	CompressionStatistics GetStatistics() const override;
	void ResetStatistics() override;
	void SetStatisticsCallback(std::function<void(const CompressionStatistics&)> callback) override;

	// 额外的配置和管理功能
	/**
	 * @brief 设置压缩器配置
	 * @param config 新的配置
	 * @note 某些配置项（如线程数）需要重启压缩器才能生效
	 */
	void SetConfig(const LogCompressorConfig& config);

	/**
	 * @brief 获取当前配置
	 * @return 当前配置
	 */
	LogCompressorConfig GetConfig() const;

	/**
	 * @brief 获取压缩器状态信息（用于调试）
	 * @return 状态信息字符串
	 */
	std::wstring GetStatusInfo() const;

private:
	// 配置和状态
	LogCompressorConfig config_;
	mutable std::mutex configMutex_;

	// 任务队列
	std::priority_queue<InternalCompressionTask, std::vector<InternalCompressionTask>, TaskComparator> taskQueue_;
	mutable std::mutex queueMutex_;
	std::condition_variable queueCondVar_;

	// 线程池管理
	std::unique_ptr<BS::thread_pool<>> threadPool_;
	std::atomic<bool> stopRequested_{ false };
	std::atomic<bool> isRunning_{ false };
	std::atomic<size_t> activeTasksCount_{ 0 };

	// 统计信息
	mutable std::mutex statsMutex_;
	CompressionStatistics statistics_;
	std::function<void(const CompressionStatistics&)> statisticsCallback_;

	// 完成通知
	mutable std::mutex completionMutex_;
	std::condition_variable completionCondVar_;

	// 内部方法

	/**
	 * @brief 处理单个压缩任务
	 * @param task 要处理的任务
	 * @return 压缩结果
	 */
	CompressionResult ProcessCompressionTask(const InternalCompressionTask& task);

	/**
	 * @brief 使用ZIP算法压缩文件
	 * @param sourceFile 源文件路径
	 * @param targetFile 目标文件路径
	 * @param result 压缩结果（输出参数）
	 * @return 压缩是否成功
	 */
	bool CompressWithZip(const std::wstring& sourceFile, const std::wstring& targetFile, CompressionResult& result);

	/**
	 * @brief 更新统计信息
	 * @param result 压缩结果
	 */
	void UpdateStatistics(const CompressionResult& result);

	/**
	 * @brief 触发统计回调
	 */
	void TriggerStatisticsCallback();

	/**
	 * @brief 验证文件路径
	 * @param filePath 文件路径
	 * @return 路径是否有效
	 */
	bool ValidateFilePath(const std::wstring& filePath) const;

	/**
	 * @brief 创建目录（如果不存在）
	 * @param filePath 文件路径
	 * @return 创建是否成功
	 */
	bool CreateDirectoryIfNotExists(const std::wstring& filePath) const;

	/**
	 * @brief 安全删除文件
	 * @param filePath 文件路径
	 * @return 删除是否成功
	 */
	bool SafeDeleteFile(const std::wstring& filePath) const;

	/**
	 * @brief 获取文件大小
	 * @param filePath 文件路径
	 * @return 文件大小，获取失败返回0
	 */
	size_t GetFileSize(const std::wstring& filePath) const;

	/**
	 * @brief 启动工作线程
	 */
	void StartWorkerThreads();

	/**
	 * @brief 停止工作线程
	 */
	void StopWorkerThreads();

	/**
	 * @brief 清理资源
	 */
	void Cleanup();
};