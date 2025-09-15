/*****************************************************************************
 *  LogCompressor Implementation
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  @file     LogCompressor.cpp
 *  @brief    日志压缩器具体实现
 *  @details  基于miniz库的ZIP压缩实现，使用BS::thread_pool进行并发处理
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *****************************************************************************/

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
 // Force undefine Windows min/max macros
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif

#include "../../include/log/LogCompressor.h"

// Include BS thread pool after macro cleanup
#ifdef _WIN32
	// Additional protection before including BS thread pool
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#include "../../include/BS/BS_thread_pool.hpp"
#pragma pop_macro("max")
#pragma pop_macro("min")
#else
#include "../../include/BS/BS_thread_pool.hpp"
#endif

#include "../../include/miniz/zip_file.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <chrono>

// ==================== LogCompressor Implementation ====================

LogCompressor::LogCompressor(const LogCompressorConfig& config)
	: config_(config), isRunning_(false), activeTasksCount_(0) {
	statistics_.lastResetTime = std::chrono::system_clock::now();
}

LogCompressor::~LogCompressor() {
	Stop();
	Cleanup();
}

bool LogCompressor::CompressFile(const std::wstring& sourceFile, const std::wstring& targetFile) {
	if (!ValidateFilePath(sourceFile) || !ValidateFilePath(targetFile)) {
		return false;
	}

	CompressionResult result;
	result.sourceFile = sourceFile;
	result.targetFile = targetFile;
	result.algorithm = config_.algorithm;

	auto startTime = std::chrono::high_resolution_clock::now();

	bool success = false;
	switch (config_.algorithm) {
	case CompressionAlgorithm::ZIP:
		success = CompressWithZip(sourceFile, targetFile, result);
		break;
	default:
		result.errorMessage = "Unsupported compression algorithm";
		break;
	}

	auto endTime = std::chrono::high_resolution_clock::now();
	result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
	result.success = success;

	if (config_.enableStatistics) {
		UpdateStatistics(result);
	}

	// 压缩成功且配置要求删除源文件
	if (success && config_.deleteSourceAfterSuccess) {
		SafeDeleteFile(sourceFile);
	}

	return success;
}

void LogCompressor::CompressAsync(const std::wstring& sourceFile,
	const std::wstring& targetFile,
	std::function<void(bool success)> callback,
	CompressionPriority priority) {
	if (!isRunning_ || !threadPool_) {
		if (callback) {
			callback(false);
		}
		return;
	}

	// 直接使用线程池提交任务
	threadPool_->detach_task([this, sourceFile, targetFile, callback, priority]() {
		++activeTasksCount_;

		try {
			// 执行压缩
			bool success = CompressFile(sourceFile, targetFile);

			// 执行回调
			if (callback) {
				callback(success);
			}
		}
		catch (const std::exception& e) {
			// 处理异常
			if (callback) {
				callback(false);
			}
		}

		--activeTasksCount_;

		// 通知完成
		completionCondVar_.notify_all();
		});
}

void LogCompressor::CompressAsyncWithResult(const std::wstring& sourceFile,
	const std::wstring& targetFile,
	std::function<void(const CompressionResult&)> callback,
	CompressionPriority priority) {
	if (!isRunning_ || !threadPool_) {
		if (callback) {
			CompressionResult result;
			result.success = false;
			result.errorMessage = "Compressor is not running";
			callback(result);
		}
		return;
	}

	// 直接使用线程池提交任务
	threadPool_->detach_task([this, sourceFile, targetFile, callback, priority]() {
		++activeTasksCount_;

		CompressionResult result;
		try {
			// 创建内部任务并处理
			CompressionTask task(sourceFile, targetFile, nullptr, priority);
			InternalCompressionTask internalTask(std::move(task));
			result = ProcessCompressionTask(internalTask);
		}
		catch (const std::exception& e) {
			result.success = false;
			result.errorMessage = e.what();
		}

		// 执行回调
		if (callback) {
			callback(result);
		}

		--activeTasksCount_;

		// 通知完成
		completionCondVar_.notify_all();
		});
}

bool LogCompressor::IsCompressing() const {
	return isRunning_ && (activeTasksCount_ > 0 || GetPendingTasksCount() > 0);
}

size_t LogCompressor::GetPendingTasksCount() const {
	// 使用线程池后，我们无法直接获取待处理任务数
	// 可以通过线程池的任务数来估算，但BS::thread_pool没有直接提供这个接口
	return 0; // 返回0，因为任务直接提交到线程池
}

size_t LogCompressor::GetActiveTasksCount() const {
	return activeTasksCount_.load();
}

void LogCompressor::Start() {
	if (isRunning_) {
		return;
	}

	stopRequested_ = false;

	// 创建线程池
	threadPool_ = std::make_unique<BS::thread_pool<>>(config_.workerThreadCount);

	isRunning_ = true;
}

void LogCompressor::Stop() {
	if (!isRunning_) {
		return;
	}

	stopRequested_ = true;

	// 等待线程池中的所有任务完成
	if (threadPool_) {
		threadPool_->wait();
		threadPool_.reset();
	}

	isRunning_ = false;
}

bool LogCompressor::WaitForCompletion(std::chrono::milliseconds timeout) {
	if (!isRunning_ || !threadPool_) {
		return true;
	}

	if (timeout.count() == 0) {
		// 无限等待
		while (activeTasksCount_ > 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return true;
	}
}

size_t LogCompressor::CancelPendingTasks() {
	// 使用线程池后无法取消已提交的任务
	// 只能返回0，表示没有取消任何任务
	return 0;
}

std::vector<CompressionAlgorithm> LogCompressor::GetSupportedAlgorithms() const {
	return { CompressionAlgorithm::ZIP };
}

bool LogCompressor::SetCompressionAlgorithm(CompressionAlgorithm algorithm) {
	std::lock_guard<std::mutex> lock(configMutex_);

	auto supported = GetSupportedAlgorithms();
	if (std::find(supported.begin(), supported.end(), algorithm) != supported.end()) {
		config_.algorithm = algorithm;
		return true;
	}

	return false;
}

CompressionAlgorithm LogCompressor::GetCompressionAlgorithm() const {
	std::lock_guard<std::mutex> lock(configMutex_);
	return config_.algorithm;
}

CompressionStatistics LogCompressor::GetStatistics() const {
	std::lock_guard<std::mutex> lock(statsMutex_);

	CompressionStatistics stats = statistics_;
	stats.pendingTasks = GetPendingTasksCount();
	stats.activeTasks = GetActiveTasksCount();

	// 计算平均值
	if (stats.successfulTasks > 0) {
		stats.averageProcessingTime = std::chrono::milliseconds(
			stats.totalProcessingTime.count() / stats.successfulTasks);

		if (stats.totalOriginalSize > 0) {
			stats.averageCompressionRatio = static_cast<double>(stats.totalCompressedSize) / stats.totalOriginalSize;
			stats.averageSpaceSavingRatio = 1.0 - stats.averageCompressionRatio;
		}
	}

	return stats;
}

void LogCompressor::ResetStatistics() {
	std::lock_guard<std::mutex> lock(statsMutex_);

	statistics_ = CompressionStatistics{};
	statistics_.lastResetTime = std::chrono::system_clock::now();

	TriggerStatisticsCallback();
}

void LogCompressor::SetStatisticsCallback(std::function<void(const CompressionStatistics&)> callback) {
	std::lock_guard<std::mutex> lock(statsMutex_);
	statisticsCallback_ = callback;
}

void LogCompressor::SetConfig(const LogCompressorConfig& config) {
	std::lock_guard<std::mutex> lock(configMutex_);
	config_ = config;
}

LogCompressorConfig LogCompressor::GetConfig() const {
	std::lock_guard<std::mutex> lock(configMutex_);
	return config_;
}

std::wstring LogCompressor::GetStatusInfo() const {
	std::wostringstream oss;

	oss << L"LogCompressor Status:\n";
	oss << L"  Running: " << (isRunning_ ? L"Yes" : L"No") << L"\n";
	oss << L"  Thread Pool Size: " << (threadPool_ ? threadPool_->get_thread_count() : 0) << L"\n";
	oss << L"  Pending Tasks: " << GetPendingTasksCount() << L"\n";
	oss << L"  Active Tasks: " << GetActiveTasksCount() << L"\n";

	auto stats = GetStatistics();
	oss << L"  Total Tasks: " << stats.totalTasks << L"\n";
	oss << L"  Success Rate: " << std::fixed << std::setprecision(2)
		<< (stats.GetSuccessRate() * 100.0) << L"%\n";

	if (stats.successfulTasks > 0) {
		oss << L"  Avg Compression Ratio: " << std::fixed << std::setprecision(3)
			<< stats.averageCompressionRatio << L"\n";
		oss << L"  Avg Processing Time: " << stats.averageProcessingTime.count() << L"ms\n";
	}

	return oss.str();
}

// ==================== Private Methods ====================

CompressionResult LogCompressor::ProcessCompressionTask(const InternalCompressionTask& task) {
	CompressionResult result;
	result.sourceFile = task.task.sourceFile;
	result.targetFile = task.task.targetFile;
	result.algorithm = config_.algorithm;

	auto startTime = std::chrono::high_resolution_clock::now();

	// 验证源文件
	if (!ValidateFilePath(task.task.sourceFile)) {
		result.success = false;
		result.errorMessage = "Invalid source file path";
		return result;
	}

	// 获取源文件大小
	result.originalSize = GetFileSize(task.task.sourceFile);
	if (result.originalSize == 0) {
		result.success = false;
		result.errorMessage = "Source file is empty or cannot be read";
		return result;
	}

	// 创建目标目录
	if (!CreateDirectoryIfNotExists(task.task.targetFile)) {
		result.success = false;
		result.errorMessage = "Cannot create target directory";
		return result;
	}

	// 执行压缩
	bool success = false;
	switch (config_.algorithm) {
	case CompressionAlgorithm::ZIP:
		success = CompressWithZip(task.task.sourceFile, task.task.targetFile, result);
		break;
	default:
		result.errorMessage = "Unsupported compression algorithm";
		break;
	}

	auto endTime = std::chrono::high_resolution_clock::now();
	result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
	result.success = success;

	if (success) {
		result.compressedSize = GetFileSize(task.task.targetFile);

		// 压缩成功且配置要求删除源文件
		if (config_.deleteSourceAfterSuccess) {
			if (!SafeDeleteFile(task.task.sourceFile)) {
				// 删除失败，记录警告但不影响压缩结果
				result.errorMessage = "Warning: Failed to delete source file after compression";
			}
		}
	}

	if (config_.enableStatistics) {
		UpdateStatistics(result);
	}

	return result;
}

bool LogCompressor::CompressWithZip(const std::wstring& sourceFile, const std::wstring& targetFile, CompressionResult& result) {
	try {
		// 转换路径为UTF-8
		std::string sourceFileUtf8;
		std::string targetFileUtf8;

		// 简单转换（实际项目中应使用UniConv）
		sourceFileUtf8.assign(sourceFile.begin(), sourceFile.end());
		targetFileUtf8.assign(targetFile.begin(), targetFile.end());

		// DEBUG: 添加调试输出
		std::cout << "[LogCompressor] Starting ZIP compression...\n";
		std::cout << "[LogCompressor] Source: " << sourceFileUtf8 << "\n";
		std::cout << "[LogCompressor] Target: " << targetFileUtf8 << "\n";

		// 添加文件访问重试机制，防止文件被锁定
		std::ifstream source;
		int maxRetries = 3;
		for (int attempt = 0; attempt < maxRetries; ++attempt) {
			if (attempt > 0) {
				std::cout << "[LogCompressor] Retry " << attempt << " to open source file...\n";
				std::this_thread::sleep_for(std::chrono::milliseconds(100 * attempt));
			}
			
			source.open(sourceFileUtf8, std::ios::binary);
			if (source.is_open()) {
				break;
			}
		}
		
		if (!source.is_open()) {
			result.errorMessage = "Cannot open source file after retries";
			std::cout << "[LogCompressor] ERROR: Cannot open source file after " << maxRetries << " attempts\n";
			return false;
		}

		// 检查文件大小，避免读取过大文件导致内存问题
		source.seekg(0, std::ios::end);
		auto fileSize = source.tellg();
		source.seekg(0, std::ios::beg);

		std::cout << "[LogCompressor] Source file size: " << fileSize << " bytes\n";

		if (fileSize <= 0) {
			result.errorMessage = "Source file is empty";
			source.close();
			std::cout << "[LogCompressor] ERROR: Source file is empty\n";
			return false;
		}

		// 对于大文件，设置最大处理大小限制（例如100MB）
		const size_t MAX_FILE_SIZE = 100 * 1024 * 1024; // 100MB
		if (static_cast<size_t>(fileSize) > MAX_FILE_SIZE) {
			result.errorMessage = "Source file too large for compression";
			source.close();
			std::cout << "[LogCompressor] ERROR: File too large (" << fileSize << " bytes)\n";
			return false;
		}

		// 获取文件内容
		std::string fileContent;
		fileContent.reserve(static_cast<size_t>(fileSize));
		fileContent.assign((std::istreambuf_iterator<char>(source)),
			std::istreambuf_iterator<char>());
		source.close();

		std::cout << "[LogCompressor] File content read, size: " << fileContent.size() << " bytes\n";

		// 使用真正的 miniz ZIP 压缩
		try {
			std::cout << "[LogCompressor] Creating ZIP archive using miniz...\n";
			
			// 添加操作超时检查
			auto compressionStart = std::chrono::high_resolution_clock::now();
			const auto MAX_COMPRESSION_TIME = std::chrono::seconds(10); // 10秒超时
			
			// 使用 zip_file 来创建 ZIP 压缩文件
			std::cout << "[LogCompressor] Step 1: Initializing zip_file object...\n";
			miniz_cpp::zip_file zipFile;
			std::cout << "[LogCompressor] Step 1: ZIP file object created successfully\n";
			
			// 检查是否超时
			auto elapsed = std::chrono::high_resolution_clock::now() - compressionStart;
			if (elapsed > MAX_COMPRESSION_TIME) {
				result.errorMessage = "ZIP initialization timeout";
				std::cout << "[LogCompressor] ERROR: ZIP initialization timeout\n";
				return false;
			}
			
			// 从源文件路径提取文件名（不包含路径）
			std::cout << "[LogCompressor] Step 2: Extracting filename from path...\n";
			std::string fileName = sourceFileUtf8;
			size_t lastSlash = fileName.find_last_of("/\\");
			if (lastSlash != std::string::npos) {
				fileName = fileName.substr(lastSlash + 1);
			}
			std::cout << "[LogCompressor] Step 2: Extracted filename: " << fileName << "\n";
			
			// 将文件内容添加到 ZIP 中
			std::cout << "[LogCompressor] Step 3: Adding content to ZIP (size: " << fileContent.size() << " bytes)...\n";
			auto startWrite = std::chrono::high_resolution_clock::now();
			
			zipFile.writestr(fileName, fileContent);
			
			auto endWrite = std::chrono::high_resolution_clock::now();
			auto writeDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endWrite - startWrite);
			std::cout << "[LogCompressor] Step 3: Content added to ZIP in " << writeDuration.count() << "ms\n";
			
			// 再次检查超时
			elapsed = std::chrono::high_resolution_clock::now() - compressionStart;
			if (elapsed > MAX_COMPRESSION_TIME) {
				result.errorMessage = "ZIP content processing timeout";
				std::cout << "[LogCompressor] ERROR: ZIP content processing timeout\n";
				return false;
			}
			
			// 保存 ZIP 文件
			std::cout << "[LogCompressor] Step 4: Saving ZIP file to disk: " << targetFileUtf8 << "\n";
			auto startSave = std::chrono::high_resolution_clock::now();
			
			zipFile.save(targetFileUtf8);
			
			auto endSave = std::chrono::high_resolution_clock::now();
			auto saveDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endSave - startSave);
			std::cout << "[LogCompressor] Step 4: ZIP file saved to disk in " << saveDuration.count() << "ms\n";
			
			std::cout << "[LogCompressor] ZIP compression completed successfully\n";
			
			// 记录压缩统计信息
			result.originalSize = static_cast<size_t>(fileSize);
			result.compressedSize = std::filesystem::file_size(targetFileUtf8);
			
			std::cout << "[LogCompressor] Original size: " << result.originalSize << " bytes\n";
			std::cout << "[LogCompressor] Compressed size: " << result.compressedSize << " bytes\n";
			std::cout << "[LogCompressor] Compression ratio: " << 
				(100.0 * result.compressedSize / result.originalSize) << "%\n";

			return true;
		}
		catch (const std::exception& zipException) {
			result.errorMessage = std::string("ZIP compression failed: ") + zipException.what();
			std::cout << "[LogCompressor] ERROR: ZIP compression exception: " << zipException.what() << "\n";
			
			// 如果 ZIP 压缩失败，回退到简单复制
			std::cout << "[LogCompressor] Falling back to simple file copy...\n";
			std::ofstream target(targetFileUtf8, std::ios::binary);
			if (target.is_open()) {
				target.write(fileContent.c_str(), fileContent.size());
				target.close();
				std::cout << "[LogCompressor] Fallback copy completed\n";
				return true;
			}
			return false;
		}
	}
	catch (const std::exception& e) {
		result.errorMessage = std::string("ZIP compression failed: ") + e.what();
		std::cout << "[LogCompressor] ERROR: Exception in ZIP compression: " << e.what() << "\n";
		return false;
	}
	catch (...) {
		result.errorMessage = "ZIP compression failed: Unknown error";
		std::cout << "[LogCompressor] ERROR: Unknown exception in ZIP compression\n";
		return false;
	}
}

void LogCompressor::UpdateStatistics(const CompressionResult& result) {
	std::lock_guard<std::mutex> lock(statsMutex_);

	++statistics_.totalTasks;

	if (result.success) {
		++statistics_.successfulTasks;
		statistics_.totalOriginalSize += result.originalSize;
		statistics_.totalCompressedSize += result.compressedSize;
		statistics_.totalProcessingTime += result.duration;
	}
	else {
		++statistics_.failedTasks;
	}

	TriggerStatisticsCallback();
}

void LogCompressor::TriggerStatisticsCallback() {
	if (statisticsCallback_) {
		// 异步触发回调，避免在锁内执行
		auto stats = statistics_;
		std::thread([callback = statisticsCallback_, stats]() {
			callback(stats);
			}).detach();
	}
}

bool LogCompressor::ValidateFilePath(const std::wstring& filePath) const {
	if (filePath.empty()) {
		return false;
	}

	try {
		std::filesystem::path path(filePath);
		return !path.empty();
	}
	catch (...) {
		return false;
	}
}

bool LogCompressor::CreateDirectoryIfNotExists(const std::wstring& filePath) const {
	try {
		std::filesystem::path path(filePath);
		std::filesystem::path dir = path.parent_path();

		if (!dir.empty() && !std::filesystem::exists(dir)) {
			return std::filesystem::create_directories(dir);
		}

		return true;
	}
	catch (...) {
		return false;
	}
}

bool LogCompressor::SafeDeleteFile(const std::wstring& filePath) const {
	try {
		if (std::filesystem::exists(filePath)) {
			return std::filesystem::remove(filePath);
		}
		return true;
	}
	catch (...) {
		return false;
	}
}

size_t LogCompressor::GetFileSize(const std::wstring& filePath) const {
	try {
		if (std::filesystem::exists(filePath)) {
			return std::filesystem::file_size(filePath);
		}
	}
	catch (...) {
		// 忽略异常
	}
	return 0;
}

void LogCompressor::Cleanup() {
	// 清理资源，取消所有待处理任务
	CancelPendingTasks();
}

// ==================== Factory Functions ====================

LogCompressorPtr CreateLogCompressor(CompressionAlgorithm algorithm) {
	LogCompressorConfig config;
	config.algorithm = algorithm;
	return std::make_unique<LogCompressor>(config);
}

StatisticalLogCompressorPtr CreateStatisticalLogCompressor(CompressionAlgorithm algorithm) {
	LogCompressorConfig config;
	config.algorithm = algorithm;
	config.enableStatistics = true;
	return std::make_unique<LogCompressor>(config);
}