#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <atomic>

/*****************************************************************************
 *  RotationPreChecker
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  @file     RotationPreChecker.h
 *  @brief    轮转预检查系统
 *  @details  在执行轮转前检查各种前置条件，防止轮转失败
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/15
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

/**
 * @brief 检查项严重级别
 */
enum class CheckSeverity {
    Info,       /*!< 信息级别 */
    Warning,    /*!< 警告级别 */
    Error,      /*!< 错误级别 */
    Critical    /*!< 严重错误级别 */
};

/**
 * @brief 检查项类型
 */
enum class CheckType {
    DiskSpace,          /*!< 磁盘空间检查 */
    FilePermissions,    /*!< 文件权限检查 */
    DirectoryAccess,    /*!< 目录访问检查 */
    FileExists,         /*!< 文件存在性检查 */
    FileLocked,         /*!< 文件锁定检查 */
    ProcessPermissions, /*!< 进程权限检查 */
    SystemResources,    /*!< 系统资源检查 */
    NetworkAccess,      /*!< 网络访问检查 */
    Custom              /*!< 自定义检查 */
};

/**
 * @brief 检查结果项
 */
struct CheckResult {
    CheckType type;                                     /*!< 检查类型 */
    CheckSeverity severity;                             /*!< 严重级别 */
    std::wstring title;                                 /*!< 检查标题 */
    std::wstring message;                               /*!< 检查消息 */
    std::wstring suggestion;                            /*!< 建议措施 */
    bool passed = false;                                /*!< 检查是否通过 */
    std::chrono::milliseconds checkDuration{0};        /*!< 检查耗时 */
    std::chrono::system_clock::time_point timestamp;   /*!< 检查时间 */
    
    CheckResult() : timestamp(std::chrono::system_clock::now()) {}
    
    CheckResult(CheckType t, CheckSeverity s, const std::wstring& title, 
               const std::wstring& msg, bool pass = false)
        : type(t), severity(s), title(title), message(msg), passed(pass)
        , timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief 预检查结果汇总
 */
struct PreCheckResult {
    bool canRotate = false;                             /*!< 是否可以执行轮转 */
    bool hasWarnings = false;                           /*!< 是否有警告 */
    bool hasErrors = false;                             /*!< 是否有错误 */
    size_t totalChecks = 0;                             /*!< 总检查项数 */
    size_t passedChecks = 0;                            /*!< 通过检查项数 */
    std::vector<CheckResult> results;                   /*!< 详细检查结果 */
    std::chrono::milliseconds totalCheckTime{0};       /*!< 总检查时间 */
    std::chrono::system_clock::time_point checkTime;   /*!< 检查开始时间 */
    
    PreCheckResult() : checkTime(std::chrono::system_clock::now()) {}
    
    /**
     * @brief 获取错误数量
     * @return 错误数量
     */
    size_t GetErrorCount() const {
        size_t count = 0;
        for (const auto& result : results) {
            if (!result.passed && (result.severity == CheckSeverity::Error || 
                                  result.severity == CheckSeverity::Critical)) {
                count++;
            }
        }
        return count;
    }
    
    /**
     * @brief 获取警告数量
     * @return 警告数量
     */
    size_t GetWarningCount() const {
        size_t count = 0;
        for (const auto& result : results) {
            if (!result.passed && result.severity == CheckSeverity::Warning) {
                count++;
            }
        }
        return count;
    }
    
    /**
     * @brief 获取成功率
     * @return 成功率 (0.0-1.0)
     */
    double GetSuccessRate() const {
        return totalChecks > 0 ? static_cast<double>(passedChecks) / totalChecks : 0.0;
    }
};

/**
 * @brief 轮转上下文信息
 */
struct RotationCheckContext {
    std::wstring sourceFile;                            /*!< 源文件路径 */
    std::wstring targetFile;                            /*!< 目标文件路径 */
    std::wstring archiveDirectory;                      /*!< 归档目录 */
    size_t estimatedFileSize = 0;                       /*!< 预估文件大小 */
    bool compressionEnabled = false;                    /*!< 是否启用压缩 */
    bool createBackup = false;                          /*!< 是否创建备份 */
    std::wstring workingDirectory;                      /*!< 工作目录 */
    
    RotationCheckContext() = default;
    
    RotationCheckContext(const std::wstring& source, const std::wstring& target)
        : sourceFile(source), targetFile(target) {}
};

/**
 * @brief 自定义检查函数类型
 */
using CustomCheckFunction = std::function<CheckResult(const RotationCheckContext&)>;

/**
 * @brief 轮转预检查器
 * @details 执行轮转前的各种检查，确保轮转条件满足
 */
class RotationPreChecker {
public:
    /**
     * @brief 构造函数
     */
    RotationPreChecker();
    
    /**
     * @brief 析构函数
     */
    ~RotationPreChecker();
    
    /**
     * @brief 执行完整的预检查
     * @param context 轮转上下文
     * @return 检查结果
     */
    PreCheckResult CheckRotationConditions(const RotationCheckContext& context);
    
    /**
     * @brief 检查磁盘空间
     * @param context 轮转上下文
     * @return 检查结果
     */
    CheckResult CheckDiskSpace(const RotationCheckContext& context);
    
    /**
     * @brief 检查文件权限
     * @param context 轮转上下文
     * @return 检查结果
     */
    CheckResult CheckFilePermissions(const RotationCheckContext& context);
    
    /**
     * @brief 检查目录访问权限
     * @param context 轮转上下文
     * @return 检查结果
     */
    CheckResult CheckDirectoryAccess(const RotationCheckContext& context);
    
    /**
     * @brief 检查文件是否存在
     * @param context 轮转上下文
     * @return 检查结果
     */
    CheckResult CheckFileExists(const RotationCheckContext& context);
    
    /**
     * @brief 检查文件是否被锁定
     * @param context 轮转上下文
     * @return 检查结果
     */
    CheckResult CheckFileLocked(const RotationCheckContext& context);
    
    /**
     * @brief 检查进程权限
     * @param context 轮转上下文
     * @return 检查结果
     */
    CheckResult CheckProcessPermissions(const RotationCheckContext& context);
    
    /**
     * @brief 检查系统资源
     * @param context 轮转上下文
     * @return 检查结果
     */
    CheckResult CheckSystemResources(const RotationCheckContext& context);
    
    /**
     * @brief 添加自定义检查函数
     * @param name 检查名称
     * @param checkFunction 检查函数
     */
    void AddCustomCheck(const std::wstring& name, CustomCheckFunction checkFunction);
    
    /**
     * @brief 移除自定义检查函数
     * @param name 检查名称
     * @return 是否移除成功
     */
    bool RemoveCustomCheck(const std::wstring& name);
    
    /**
     * @brief 设置磁盘空间阈值
     * @param thresholdMB 阈值(MB)
     */
    void SetDiskSpaceThreshold(size_t thresholdMB);
    
    /**
     * @brief 设置内存使用阈值
     * @param thresholdPercent 阈值百分比(0-100)
     */
    void SetMemoryThreshold(size_t thresholdPercent);
    
    /**
     * @brief 设置检查超时时间
     * @param timeout 超时时间
     */
    void SetCheckTimeout(std::chrono::milliseconds timeout);
    
    /**
     * @brief 启用/禁用特定检查类型
     * @param checkType 检查类型
     * @param enabled 是否启用
     */
    void SetCheckEnabled(CheckType checkType, bool enabled);
    
    /**
     * @brief 检查特定检查类型是否启用
     * @param checkType 检查类型
     * @return 是否启用
     */
    bool IsCheckEnabled(CheckType checkType) const;
    
    /**
     * @brief 获取检查统计信息
     * @return 统计信息字符串
     */
    std::wstring GetCheckStatistics() const;
    
    /**
     * @brief 重置统计信息
     */
    void ResetStatistics();

private:
    // 配置参数
    size_t diskSpaceThresholdMB_ = 1024;               /*!< 磁盘空间阈值(MB) */
    size_t memoryThresholdPercent_ = 80;               /*!< 内存使用阈值(%) */
    std::chrono::milliseconds checkTimeout_{5000};    /*!< 检查超时时间 */
    
    // 检查开关
    std::map<CheckType, bool> checkEnabled_;           /*!< 检查类型开关 */
    
    // 自定义检查
    std::map<std::wstring, CustomCheckFunction> customChecks_; /*!< 自定义检查函数 */
    
    // 统计信息
    std::atomic<size_t> totalChecks_{0};              /*!< 总检查次数 */
    std::atomic<size_t> successfulChecks_{0};         /*!< 成功检查次数 */
    std::atomic<size_t> failedChecks_{0};             /*!< 失败检查次数 */
    std::atomic<long long> totalCheckTimeMs_{0};          /*!< 总检查时间(毫秒) */
    
    /**
     * @brief 初始化默认检查开关
     */
    void InitializeDefaultChecks();
    
    /**
     * @brief 获取可用磁盘空间
     * @param path 路径
     * @return 可用空间(字节)
     */
    uintmax_t GetAvailableDiskSpace(const std::wstring& path) const;
    
    /**
     * @brief 获取系统内存使用情况
     * @return 内存使用百分比(0-100)
     */
    size_t GetMemoryUsagePercent() const;
    
    /**
     * @brief 检查文件是否可读
     * @param filePath 文件路径
     * @return 是否可读
     */
    bool IsFileReadable(const std::wstring& filePath) const;
    
    /**
     * @brief 检查文件是否可写
     * @param filePath 文件路径
     * @return 是否可写
     */
    bool IsFileWritable(const std::wstring& filePath) const;
    
    /**
     * @brief 检查目录是否可写
     * @param dirPath 目录路径
     * @return 是否可写
     */
    bool IsDirectoryWritable(const std::wstring& dirPath) const;
    
    /**
     * @brief 检查文件是否被其他进程锁定
     * @param filePath 文件路径
     * @return 是否被锁定
     */
    bool IsFileLocked(const std::wstring& filePath) const;
    
    /**
     * @brief 执行单个检查项
     * @param checkFunction 检查函数
     * @param context 轮转上下文
     * @return 检查结果
     */
    CheckResult ExecuteCheck(std::function<CheckResult(const RotationCheckContext&)> checkFunction,
                           const RotationCheckContext& context);
    
    /**
     * @brief 更新统计信息
     * @param result 检查结果
     */
    void UpdateStatistics(const CheckResult& result);
    
    /**
     * @brief 生成检查建议
     * @param checkType 检查类型
     * @param severity 严重级别
     * @return 建议字符串
     */
    std::wstring GenerateSuggestion(CheckType checkType, CheckSeverity severity) const;
};

/**
 * @brief 预检查器工厂类
 */
class PreCheckerFactory {
public:
    /**
     * @brief 创建标准预检查器
     * @return 预检查器实例
     */
    static std::unique_ptr<RotationPreChecker> CreateStandard();
    
    /**
     * @brief 创建快速检查器（只执行关键检查）
     * @return 预检查器实例
     */
    static std::unique_ptr<RotationPreChecker> CreateFast();
    
    /**
     * @brief 创建全面检查器（执行所有检查）
     * @return 预检查器实例
     */
    static std::unique_ptr<RotationPreChecker> CreateComprehensive();
    
    /**
     * @brief 创建自定义检查器
     * @param enabledChecks 启用的检查类型列表
     * @return 预检查器实例
     */
    static std::unique_ptr<RotationPreChecker> CreateCustom(
        const std::vector<CheckType>& enabledChecks);
};