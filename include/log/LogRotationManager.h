#pragma once

#include "ILogRotationManager.h"
#include "ILogCompressor.h"
#include <filesystem>
#include <mutex>
#include <atomic>
#include <vector>
#include <algorithm>

/*****************************************************************************
 *  LogRotationManager
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     LogRotationManager.h
 *  @brief    日志轮转管理器具体实现
 *  @details  基于策略模式的日志轮转管理器，支持多种轮转策略和压缩
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/15
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

/**
 * @brief 日志轮转管理器具体实现
 */
class LogRotationManager : public ILogRotationManager {
public:
    /**
     * @brief 构造函数
     * @param config 轮转配置
     * @param compressor 压缩器（可选）
     */
    explicit LogRotationManager(const LogRotationConfig& config = LogRotationConfig{},
                               std::shared_ptr<ILogCompressor> compressor = nullptr);
    
    /**
     * @brief 析构函数
     */
    virtual ~LogRotationManager();
    
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
    
    // 扩展功能
    /**
     * @brief 设置压缩器
     * @param compressor 压缩器实例
     */
    void SetCompressor(std::shared_ptr<ILogCompressor> compressor);
    
    /**
     * @brief 获取压缩器
     * @return 压缩器实例
     */
    std::shared_ptr<ILogCompressor> GetCompressor() const;
    
    /**
     * @brief 验证轮转配置
     * @param config 要验证的配置
     * @return 配置是否有效
     */
    static bool ValidateConfig(const LogRotationConfig& config);
    
    /**
     * @brief 获取轮转管理器状态信息（用于调试）
     * @return 状态信息字符串
     */
    std::wstring GetStatusInfo() const;

private:
    // 配置和状态
    mutable std::mutex configMutex_;
    LogRotationConfig config_;
    std::shared_ptr<ILogCompressor> compressor_;
    
    // 统计信息
    mutable std::mutex statsMutex_;
    RotationStatistics statistics_;
    
    // 回调管理
    mutable std::mutex callbackMutex_;
    RotationCallback rotationCallback_;
    
    // 运行状态
    std::atomic<bool> isRunning_{false};
    std::atomic<bool> stopRequested_{false};
    
    // 时间管理
    mutable std::mutex timeMutex_;
    std::chrono::system_clock::time_point lastRotationTime_;
    
    // 内部方法
    
    /**
     * @brief 检查文件大小是否超限
     * @param fileSize 当前文件大小
     * @return 是否超限
     */
    bool CheckSizeLimit(size_t fileSize) const;
    
    /**
     * @brief 检查时间间隔是否到达
     * @return 是否到达轮转时间
     */
    bool CheckTimeInterval() const;
    
    /**
     * @brief 生成归档文件名
     * @param currentFileName 当前文件名
     * @param timestamp 时间戳
     * @return 归档文件名
     */
    std::wstring GenerateArchiveFileName(const std::wstring& currentFileName,
                                        const std::chrono::system_clock::time_point& timestamp) const;
    
    /**
     * @brief 执行文件轮转操作
     * @param currentFileName 当前文件名
     * @param archiveFileName 归档文件名
     * @param enableCompression 是否启用压缩
     * @return 轮转是否成功
     */
    bool RotateFile(const std::wstring& currentFileName,
                   const std::wstring& archiveFileName,
                   bool enableCompression);
    
    /**
     * @brief 更新统计信息
     * @param result 轮转结果
     */
    void UpdateStatistics(const RotationResult& result);
    
    /**
     * @brief 触发轮转回调
     * @param result 轮转结果
     */
    void TriggerCallback(const RotationResult& result);
    
    /**
     * @brief 确保目录存在
     * @param dirPath 目录路径
     * @return 创建是否成功
     */
    bool EnsureDirectoryExists(const std::wstring& dirPath) const;
    
    /**
     * @brief 获取文件大小
     * @param filePath 文件路径
     * @return 文件大小，失败返回0
     */
    size_t GetFileSize(const std::wstring& filePath) const;
    
    /**
     * @brief 获取文件的基础名称（不含路径和扩展名）
     * @param filePath 文件路径
     * @return 基础名称
     */
    std::wstring GetBaseName(const std::wstring& filePath) const;
    
    /**
     * @brief 安全删除文件
     * @param filePath 文件路径
     * @return 删除是否成功
     */
    bool SafeDeleteFile(const std::wstring& filePath) const;
    
    /**
     * @brief 计算下次轮转时间
     * @return 下次轮转时间
     */
    std::chrono::system_clock::time_point CalculateNextRotationTime() const;
    
    /**
     * @brief 获取轮转间隔持续时间
     * @return 间隔持续时间
     */
    std::chrono::hours GetIntervalDuration() const;
};

/**
 * @brief 轮转管理器工厂类
 */
class LogRotationManagerFactory {
public:
    /**
     * @brief 创建标准轮转管理器
     * @param config 轮转配置
     * @param compressor 压缩器（可选）
     * @return 轮转管理器实例
     */
    static LogRotationManagerPtr CreateStandard(const LogRotationConfig& config = LogRotationConfig{},
                                               std::shared_ptr<ILogCompressor> compressor = nullptr);
    
    /**
     * @brief 创建高性能轮转管理器（针对高并发场景优化）
     * @param config 轮转配置
     * @param compressor 压缩器（可选）
     * @return 轮转管理器实例
     */
    static LogRotationManagerPtr CreateHighPerformance(const LogRotationConfig& config = LogRotationConfig{},
                                                      std::shared_ptr<ILogCompressor> compressor = nullptr);
    
    /**
     * @brief 从配置文件创建轮转管理器
     * @param configFilePath 配置文件路径
     * @param compressor 压缩器（可选）
     * @return 轮转管理器实例
     */
    static LogRotationManagerPtr CreateFromConfigFile(const std::wstring& configFilePath,
                                                     std::shared_ptr<ILogCompressor> compressor = nullptr);
};

// 实现工厂函数
inline LogRotationManagerPtr CreateLogRotationManager(const LogRotationConfig& config) {
    return std::make_unique<LogRotationManager>(config);
}