#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <memory>
#include <vector>

/*****************************************************************************
 *  ILogCompressor Interface
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3 as
 *  published by the Free Software Foundation.
 *
 *  @file     ILogCompressor.h
 *  @brief    日志压缩器抽象接口
 *  @details  定义日志文件压缩功能的统一接口，支持多种压缩算法
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/15
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

/**
 * @brief 压缩算法枚举
 */
enum class CompressionAlgorithm {
    ZIP,        /*!< ZIP压缩算法 (基于miniz) */
    GZIP,       /*!< GZIP压缩算法 */
    LZ4,        /*!< LZ4快速压缩算法 */
    ZSTD        /*!< Zstandard压缩算法 */
};

/**
 * @brief 压缩任务优先级
 */
enum class CompressionPriority {
    Low,        /*!< 低优先级 */
    Normal,     /*!< 普通优先级 */
    High        /*!< 高优先级 */
};

/**
 * @brief 压缩任务状态
 */
enum class CompressionTaskStatus {
    Pending,    /*!< 等待处理 */
    Processing, /*!< 正在处理 */
    Completed,  /*!< 已完成 */
    Failed      /*!< 失败 */
};

/**
 * @brief 压缩任务信息
 */
struct CompressionTask {
    std::wstring sourceFile;                                    /*!< 源文件路径 */
    std::wstring targetFile;                                    /*!< 目标文件路径 */
    std::chrono::system_clock::time_point createdTime;         /*!< 任务创建时间 */
    std::function<void(bool success)> callback;                /*!< 完成回调函数 */
    CompressionPriority priority = CompressionPriority::Normal; /*!< 任务优先级 */
    CompressionTaskStatus status = CompressionTaskStatus::Pending; /*!< 任务状态 */
    
    CompressionTask() = default;
    CompressionTask(const std::wstring& src, const std::wstring& dst, 
                   std::function<void(bool)> cb = nullptr,
                   CompressionPriority prio = CompressionPriority::Normal)
        : sourceFile(src), targetFile(dst), callback(std::move(cb)), priority(prio)
        , createdTime(std::chrono::system_clock::now()) {}
};

/**
 * @brief 压缩结果信息
 */
struct CompressionResult {
    bool success;                                               /*!< 是否成功 */
    std::wstring sourceFile;                                    /*!< 源文件路径 */
    std::wstring targetFile;                                    /*!< 目标文件路径 */
    size_t originalSize;                                        /*!< 原始文件大小 */
    size_t compressedSize;                                      /*!< 压缩后大小 */
    std::chrono::milliseconds duration;                        /*!< 压缩耗时 */
    std::string errorMessage;                                   /*!< 错误消息 */
    CompressionAlgorithm algorithm;                             /*!< 使用的压缩算法 */
    
    /**
     * @brief 获取压缩比率
     * @return 压缩比率 (压缩后大小/原始大小)
     */
    double GetCompressionRatio() const {
        return originalSize > 0 ? static_cast<double>(compressedSize) / originalSize : 0.0;
    }
    
    /**
     * @brief 获取空间节省比率
     * @return 节省空间比率 (节省的空间/原始大小)
     */
    double GetSpaceSavingRatio() const {
        return originalSize > 0 ? static_cast<double>(originalSize - compressedSize) / originalSize : 0.0;
    }
    
    CompressionResult() : success(false), originalSize(0), compressedSize(0)
                        , duration(0), algorithm(CompressionAlgorithm::ZIP) {}
};

/**
 * @brief 日志压缩器抽象接口
 * @details 定义日志文件压缩功能的统一接口，支持同步和异步压缩，
 *          提供任务管理和统计功能
 */
class ILogCompressor {
public:
    virtual ~ILogCompressor() = default;
    
    /**
     * @brief 同步压缩文件
     * @param sourceFile 源文件路径
     * @param targetFile 目标文件路径
     * @return 压缩是否成功
     * @details 阻塞方式压缩单个文件，适用于简单场景
     */
    virtual bool CompressFile(const std::wstring& sourceFile, 
                             const std::wstring& targetFile) = 0;
    
    /**
     * @brief 异步压缩文件
     * @param sourceFile 源文件路径
     * @param targetFile 目标文件路径  
     * @param callback 完成时的回调函数，参数为成功标志
     * @param priority 任务优先级
     * @details 非阻塞方式压缩文件，任务加入队列后立即返回
     */
    virtual void CompressAsync(const std::wstring& sourceFile, 
                              const std::wstring& targetFile,
                              std::function<void(bool success)> callback = nullptr,
                              CompressionPriority priority = CompressionPriority::Normal) = 0;
    
    /**
     * @brief 异步压缩文件（返回详细结果）
     * @param sourceFile 源文件路径
     * @param targetFile 目标文件路径
     * @param callback 完成时的回调函数，参数为压缩结果
     * @param priority 任务优先级
     */
    virtual void CompressAsyncWithResult(const std::wstring& sourceFile,
                                        const std::wstring& targetFile,
                                        std::function<void(const CompressionResult&)> callback,
                                        CompressionPriority priority = CompressionPriority::Normal) = 0;
    
    /**
     * @brief 检查是否正在压缩
     * @return 是否有任务正在执行
     */
    virtual bool IsCompressing() const = 0;
    
    /**
     * @brief 获取待处理任务数量
     * @return 队列中等待处理的任务数量
     */
    virtual size_t GetPendingTasksCount() const = 0;
    
    /**
     * @brief 获取正在处理的任务数量
     * @return 当前正在执行的任务数量
     */
    virtual size_t GetActiveTasksCount() const = 0;
    
    /**
     * @brief 启动压缩器
     * @details 启动工作线程，开始处理压缩任务
     */
    virtual void Start() = 0;
    
    /**
     * @brief 停止压缩器
     * @details 停止接受新任务，等待当前任务完成后关闭工作线程
     */
    virtual void Stop() = 0;
    
    /**
     * @brief 等待所有任务完成
     * @param timeout 超时时间，0表示无限等待
     * @return 是否在超时前完成所有任务
     */
    virtual bool WaitForCompletion(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) = 0;
    
    /**
     * @brief 取消所有待处理任务
     * @return 被取消的任务数量
     */
    virtual size_t CancelPendingTasks() = 0;
    
    /**
     * @brief 获取支持的压缩算法
     * @return 支持的算法列表
     */
    virtual std::vector<CompressionAlgorithm> GetSupportedAlgorithms() const = 0;
    
    /**
     * @brief 设置压缩算法
     * @param algorithm 要使用的压缩算法
     * @return 设置是否成功
     */
    virtual bool SetCompressionAlgorithm(CompressionAlgorithm algorithm) = 0;
    
    /**
     * @brief 获取当前压缩算法
     * @return 当前使用的压缩算法
     */
    virtual CompressionAlgorithm GetCompressionAlgorithm() const = 0;
};

/**
 * @brief 压缩器统计信息
 */
struct CompressionStatistics {
    size_t totalTasks = 0;                              /*!< 总任务数 */
    size_t successfulTasks = 0;                         /*!< 成功任务数 */
    size_t failedTasks = 0;                             /*!< 失败任务数 */
    size_t pendingTasks = 0;                            /*!< 待处理任务数 */
    size_t activeTasks = 0;                             /*!< 正在处理任务数 */
    
    size_t totalOriginalSize = 0;                       /*!< 总原始文件大小 */
    size_t totalCompressedSize = 0;                     /*!< 总压缩后大小 */
    
    std::chrono::milliseconds totalProcessingTime{0};   /*!< 总处理时间 */
    std::chrono::milliseconds averageProcessingTime{0}; /*!< 平均处理时间 */
    
    double averageCompressionRatio = 0.0;               /*!< 平均压缩比率 */
    double averageSpaceSavingRatio = 0.0;               /*!< 平均空间节省比率 */
    
    std::chrono::system_clock::time_point lastResetTime; /*!< 上次重置统计的时间 */
    
    /**
     * @brief 计算成功率
     * @return 任务成功率 (0.0-1.0)
     */
    double GetSuccessRate() const {
        return totalTasks > 0 ? static_cast<double>(successfulTasks) / totalTasks : 0.0;
    }
    
    /**
     * @brief 计算总体压缩效果
     * @return 总体压缩比率
     */
    double GetOverallCompressionRatio() const {
        return totalOriginalSize > 0 ? static_cast<double>(totalCompressedSize) / totalOriginalSize : 0.0;
    }
    
    CompressionStatistics() : lastResetTime(std::chrono::system_clock::now()) {}
};

/**
 * @brief 可统计的压缩器接口
 * @details 扩展基础压缩器接口，增加统计功能
 */
class IStatisticalLogCompressor : public ILogCompressor {
public:
    /**
     * @brief 获取统计信息
     * @return 当前统计信息
     */
    virtual CompressionStatistics GetStatistics() const = 0;
    
    /**
     * @brief 重置统计信息
     */
    virtual void ResetStatistics() = 0;
    
    /**
     * @brief 设置统计回调
     * @param callback 统计更新时的回调函数
     */
    virtual void SetStatisticsCallback(std::function<void(const CompressionStatistics&)> callback) = 0;
};

/**
 * @brief 智能指针类型定义
 */
using LogCompressorPtr = std::unique_ptr<ILogCompressor>;
using StatisticalLogCompressorPtr = std::unique_ptr<IStatisticalLogCompressor>;

/**
 * @brief 创建默认的日志压缩器工厂函数
 * @param algorithm 压缩算法
 * @return 压缩器实例
 */
LogCompressorPtr CreateLogCompressor(CompressionAlgorithm algorithm = CompressionAlgorithm::ZIP);

/**
 * @brief 创建带统计功能的日志压缩器工厂函数
 * @param algorithm 压缩算法
 * @return 带统计功能的压缩器实例
 */
StatisticalLogCompressorPtr CreateStatisticalLogCompressor(CompressionAlgorithm algorithm = CompressionAlgorithm::ZIP);