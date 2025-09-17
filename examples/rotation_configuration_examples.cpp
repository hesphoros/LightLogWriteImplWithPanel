/**
 * @file rotation_configuration_examples.cpp
 * @brief 日志轮转配置示例集合
 * @details 展示不同应用场景下的最佳配置实践
 * @author hesphoros
 * @date 2025/09/16
 * @version 1.0.0
 */

#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include "log/LightLogWriteImpl.h"
#include "log/ILogRotationManager.h"
#include "log/RotationManagerFactory.h"
#include "log/LogCompressor.h"

/**
 * @brief 配置预设集合类
 * 提供各种常见场景的预配置轮转策略
 */
class RotationConfigPresets {
public:
    /**
     * @brief 开发环境配置
     * 特点：快速轮转，不压缩，便于调试
     */
    static LogRotationConfig GetDevelopmentConfig() {
        LogRotationConfig config;
        config.enabled = true;
        config.strategy = LogRotationStrategy::Size;
        config.maxFileSizeMB = 10;              // 小文件，快速轮转
        config.maxBackupFiles = 3;              // 少量备份
        config.compressionEnabled = false;      // 不压缩，便于直接查看
        config.asyncRotation = false;           // 同步轮转，便于调试
        config.autoCleanup = true;              // 自动清理
        config.cleanupOlderThanDays = 7;        // 只保留一周
        config.rotatedFilePattern = L"{basename}_{index}.{extension}";
        config.useSequentialNumbers = true;    // 使用序号而非时间戳
        
        return config;
    }
    
    /**
     * @brief 生产环境配置
     * 特点：平衡性能和存储，可靠性优先
     */
    static LogRotationConfig GetProductionConfig() {
        LogRotationConfig config;
        config.enabled = true;
        config.strategy = LogRotationStrategy::SizeAndTime;
        config.maxFileSizeMB = 100;             // 中等大小文件
        config.timeInterval = TimeRotationInterval::Daily;
        config.maxBackupFiles = 30;             // 保留一个月
        config.compressionEnabled = true;       // 启用压缩节省空间
        config.compressionLevel = 6;            // 平衡压缩比和速度
        config.asyncRotation = true;            // 异步轮转，不影响性能
        config.asyncCompression = true;         // 异步压缩
        config.archiveOldFiles = true;          // 归档旧文件
        config.archivePath = L"logs/archive";
        config.autoCleanup = true;              // 自动清理
        config.cleanupOlderThanDays = 90;       // 清理90天前的日志
        config.diskSpaceThresholdMB = 1024;     // 1GB 磁盘阈值
        config.preserveFileAttributes = true;   // 保留文件属性
        config.rotatedFilePattern = L"{basename}_{timestamp}.{extension}";
        config.timestampFormat = L"%Y%m%d_%H%M%S";
        
        return config;
    }
    
    /**
     * @brief 高频日志配置
     * 特点：应对高并发写入，优化性能
     */
    static LogRotationConfig GetHighVolumeConfig() {
        LogRotationConfig config;
        config.enabled = true;
        config.strategy = LogRotationStrategy::Size;
        config.maxFileSizeMB = 200;             // 大文件减少轮转频率
        config.maxBackupFiles = 20;             // 更多备份
        config.compressionEnabled = true;       // 压缩节省空间
        config.compressionLevel = 1;            // 快速压缩，降低CPU负载
        config.asyncRotation = true;            // 异步轮转必须启用
        config.asyncCompression = true;         // 异步压缩
        config.diskSpaceThresholdMB = 2048;     // 2GB 磁盘阈值
        config.autoCleanup = true;
        config.cleanupOlderThanDays = 30;       // 30天清理
        config.rotatedFilePattern = L"{basename}_{timestamp}_{size}.{extension}";
        config.timestampFormat = L"%Y%m%d_%H%M%S";
        
        return config;
    }
    
    /**
     * @brief 合规性配置
     * 特点：长期保存，安全删除，完整归档
     */
    static LogRotationConfig GetComplianceConfig() {
        LogRotationConfig config;
        config.enabled = true;
        config.strategy = LogRotationStrategy::Time;
        config.timeInterval = TimeRotationInterval::Daily;
        config.maxBackupFiles = 365;            // 保留一年
        config.compressionEnabled = true;       // 压缩以节省长期存储空间
        config.compressionLevel = 9;            // 最高压缩比
        config.asyncRotation = false;           // 同步轮转确保完整性
        config.asyncCompression = false;        // 同步压缩确保完整性
        config.archiveOldFiles = true;          // 必须归档
        config.archivePath = L"logs/compliance_archive";
        config.autoCleanup = false;             // 不自动清理，手动管理
        config.preserveFileAttributes = true;   // 保留所有属性
        config.secureDelete = true;             // 安全删除
        config.rotatedFilePattern = L"{basename}_{timestamp}_compliance.{extension}";
        config.timestampFormat = L"%Y%m%d_%H%M%S";
        
        return config;
    }
    
    /**
     * @brief 空间受限配置
     * 特点：积极压缩和清理，最小化存储使用
     */
    static LogRotationConfig GetSpaceConstrainedConfig() {
        LogRotationConfig config;
        config.enabled = true;
        config.strategy = LogRotationStrategy::Size;
        config.maxFileSizeMB = 20;              // 小文件
        config.maxBackupFiles = 5;              // 少量备份
        config.compressionEnabled = true;       // 必须压缩
        config.compressionLevel = 9;            // 最高压缩比
        config.asyncRotation = true;
        config.asyncCompression = true;
        config.autoCleanup = true;              // 积极清理
        config.cleanupOlderThanDays = 7;        // 7天清理
        config.diskSpaceThresholdMB = 100;      // 低磁盘阈值
        config.rotatedFilePattern = L"{basename}_{index}.zip";
        config.useSequentialNumbers = true;
        
        return config;
    }
    
    /**
     * @brief 调试模式配置
     * 特点：频繁轮转，详细文件名，便于排查问题
     */
    static LogRotationConfig GetDebugConfig() {
        LogRotationConfig config;
        config.enabled = true;
        config.strategy = LogRotationStrategy::Size;
        config.maxFileSizeMB = 5;               // 很小的文件
        config.maxBackupFiles = 10;             // 中等备份数
        config.compressionEnabled = false;      // 不压缩便于查看
        config.asyncRotation = false;           // 同步轮转便于调试
        config.autoCleanup = true;
        config.cleanupOlderThanDays = 3;        // 快速清理
        config.rotatedFilePattern = L"{basename}_{timestamp}_{size}b_debug.{extension}";
        config.timestampFormat = L"%Y%m%d_%H%M%S";
        
        return config;
    }
};

/**
 * @brief 展示各种配置预设的使用
 */
void DemonstrateConfigPresets() {
    std::wcout << L"=== 日志轮转配置预设演示 ===\n\n";
    
    auto compressor = std::make_shared<LogCompressor>();
    
    // 存储不同配置的管理器
    std::map<std::wstring, std::unique_ptr<ILogRotationManager>> managers;
    
    try {
        // 1. 开发环境配置
        std::wcout << L"1. 开发环境配置:\n";
        auto devConfig = RotationConfigPresets::GetDevelopmentConfig();
        managers[L"development"] = RotationManagerFactory::CreateAsyncRotationManager(devConfig, compressor);
        
        std::wcout << L"   - 策略: 基于大小 (" << devConfig.maxFileSizeMB << L" MB)\n";
        std::wcout << L"   - 备份数: " << devConfig.maxBackupFiles << L"\n";
        std::wcout << L"   - 压缩: " << (devConfig.compressionEnabled ? L"启用" : L"禁用") << L"\n";
        std::wcout << L"   - 清理周期: " << devConfig.cleanupOlderThanDays << L" 天\n\n";
        
        // 2. 生产环境配置
        std::wcout << L"2. 生产环境配置:\n";
        auto prodConfig = RotationConfigPresets::GetProductionConfig();
        managers[L"production"] = RotationManagerFactory::CreateAsyncRotationManager(prodConfig, compressor);
        
        std::wcout << L"   - 策略: 复合策略 (" << prodConfig.maxFileSizeMB << L" MB + 每天)\n";
        std::wcout << L"   - 备份数: " << prodConfig.maxBackupFiles << L"\n";
        std::wcout << L"   - 压缩级别: " << prodConfig.compressionLevel << L"\n";
        std::wcout << L"   - 归档: " << prodConfig.archivePath << L"\n";
        std::wcout << L"   - 磁盘阈值: " << prodConfig.diskSpaceThresholdMB << L" MB\n\n";
        
        // 3. 高频日志配置
        std::wcout << L"3. 高频日志配置:\n";
        auto highVolumeConfig = RotationConfigPresets::GetHighVolumeConfig();
        managers[L"high_volume"] = RotationManagerFactory::CreateAsyncRotationManager(highVolumeConfig, compressor);
        
        std::wcout << L"   - 策略: 基于大小 (" << highVolumeConfig.maxFileSizeMB << L" MB)\n";
        std::wcout << L"   - 备份数: " << highVolumeConfig.maxBackupFiles << L"\n";
        std::wcout << L"   - 快速压缩: 级别 " << highVolumeConfig.compressionLevel << L"\n";
        std::wcout << L"   - 异步操作: 启用\n\n";
        
        // 4. 合规性配置
        std::wcout << L"4. 合规性配置:\n";
        auto complianceConfig = RotationConfigPresets::GetComplianceConfig();
        managers[L"compliance"] = RotationManagerFactory::CreateAsyncRotationManager(complianceConfig, compressor);
        
        std::wcout << L"   - 策略: 基于时间 (每天)\n";
        std::wcout << L"   - 备份数: " << complianceConfig.maxBackupFiles << L" (一年)\n";
        std::wcout << L"   - 高压缩比: 级别 " << complianceConfig.compressionLevel << L"\n";
        std::wcout << L"   - 安全删除: " << (complianceConfig.secureDelete ? L"启用" : L"禁用") << L"\n";
        std::wcout << L"   - 自动清理: " << (complianceConfig.autoCleanup ? L"启用" : L"禁用") << L"\n\n";
        
        // 5. 空间受限配置
        std::wcout << L"5. 空间受限配置:\n";
        auto spaceConfig = RotationConfigPresets::GetSpaceConstrainedConfig();
        managers[L"space_constrained"] = RotationManagerFactory::CreateAsyncRotationManager(spaceConfig, compressor);
        
        std::wcout << L"   - 策略: 基于大小 (" << spaceConfig.maxFileSizeMB << L" MB)\n";
        std::wcout << L"   - 备份数: " << spaceConfig.maxBackupFiles << L" (最少)\n";
        std::wcout << L"   - 最高压缩: 级别 " << spaceConfig.compressionLevel << L"\n";
        std::wcout << L"   - 快速清理: " << spaceConfig.cleanupOlderThanDays << L" 天\n";
        std::wcout << L"   - 磁盘阈值: " << spaceConfig.diskSpaceThresholdMB << L" MB\n\n";
        
        // 启动所有管理器并设置回调
        for (auto& [name, manager] : managers) {
            manager->SetRotationCallback([name](const RotationResult& result) {
                std::wcout << L"[" << name << L"] 轮转" 
                          << (result.success ? L"成功" : L"失败") 
                          << L": " << result.newFileName << L"\n";
            });
            manager->Start();
        }
        
        // 模拟不同场景的轮转测试
        std::wcout << L"=== 轮转测试 ===\n";
        
        // 测试开发环境 - 小文件快速轮转
        std::wcout << L"测试开发环境配置...\n";
        auto devTrigger = managers[L"development"]->CheckRotationNeeded(L"logs/dev.log", 12 * 1024 * 1024);
        if (devTrigger.sizeExceeded) {
            managers[L"development"]->ForceRotation(L"logs/dev.log", L"Development test");
        }
        
        // 测试生产环境 - 复合策略
        std::wcout << L"测试生产环境配置...\n";
        auto prodTrigger = managers[L"production"]->CheckRotationNeeded(L"logs/prod.log", 80 * 1024 * 1024);
        if (prodTrigger.sizeExceeded || prodTrigger.timeReached) {
            managers[L"production"]->ForceRotation(L"logs/prod.log", L"Production test");
        }
        
        // 测试高频日志 - 大文件处理
        std::wcout << L"测试高频日志配置...\n";
        auto highVolTrigger = managers[L"high_volume"]->CheckRotationNeeded(L"logs/high_vol.log", 180 * 1024 * 1024);
        if (highVolTrigger.sizeExceeded) {
            managers[L"high_volume"]->ForceRotation(L"logs/high_vol.log", L"High volume test");
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // 收集统计信息
        std::wcout << L"\n=== 轮转统计 ===\n";
        for (auto& [name, manager] : managers) {
            auto stats = manager->GetStatistics();
            std::wcout << L"[" << name << L"]:\n";
            std::wcout << L"  总轮转次数: " << stats.totalRotations << L"\n";
            std::wcout << L"  成功次数: " << stats.successfulRotations << L"\n";
            std::wcout << L"  平均时间: " << stats.averageRotationTimeMs << L" ms\n";
            if (stats.totalSpaceSavedMB > 0) {
                std::wcout << L"  节省空间: " << stats.totalSpaceSavedMB << L" MB\n";
            }
            std::wcout << L"\n";
        }
        
        // 停止所有管理器
        for (auto& [name, manager] : managers) {
            manager->Stop();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Configuration demonstration error: " << e.what() << std::endl;
    }
}

/**
 * @brief 自定义配置构建器
 * 提供流式接口来构建自定义配置
 */
class RotationConfigBuilder {
private:
    LogRotationConfig config_;
    
public:
    RotationConfigBuilder() {
        // 设置默认值
        config_.enabled = true;
        config_.strategy = LogRotationStrategy::Size;
        config_.maxFileSizeMB = 50;
        config_.maxBackupFiles = 10;
        config_.compressionEnabled = false;
        config_.asyncRotation = true;
    }
    
    RotationConfigBuilder& Strategy(LogRotationStrategy strategy) {
        config_.strategy = strategy;
        return *this;
    }
    
    RotationConfigBuilder& MaxSize(size_t sizeMB) {
        config_.maxFileSizeMB = sizeMB;
        return *this;
    }
    
    RotationConfigBuilder& MaxBackups(size_t count) {
        config_.maxBackupFiles = count;
        return *this;
    }
    
    RotationConfigBuilder& TimeInterval(TimeRotationInterval interval) {
        config_.timeInterval = interval;
        return *this;
    }
    
    RotationConfigBuilder& EnableCompression(bool enable = true, int level = 6) {
        config_.compressionEnabled = enable;
        config_.compressionLevel = level;
        return *this;
    }
    
    RotationConfigBuilder& AsyncMode(bool enable = true) {
        config_.asyncRotation = enable;
        config_.asyncCompression = enable;
        return *this;
    }
    
    RotationConfigBuilder& Archive(const std::wstring& path) {
        config_.archiveOldFiles = true;
        config_.archivePath = path;
        return *this;
    }
    
    RotationConfigBuilder& AutoCleanup(int days) {
        config_.autoCleanup = true;
        config_.cleanupOlderThanDays = days;
        return *this;
    }
    
    RotationConfigBuilder& DiskThreshold(size_t thresholdMB) {
        config_.diskSpaceThresholdMB = thresholdMB;
        return *this;
    }
    
    RotationConfigBuilder& FilePattern(const std::wstring& pattern) {
        config_.rotatedFilePattern = pattern;
        return *this;
    }
    
    RotationConfigBuilder& TimestampFormat(const std::wstring& format) {
        config_.timestampFormat = format;
        return *this;
    }
    
    LogRotationConfig Build() const {
        return config_;
    }
};

/**
 * @brief 展示配置构建器的使用
 */
void DemonstrateConfigBuilder() {
    std::wcout << L"=== 配置构建器演示 ===\n\n";
    
    try {
        // 使用构建器创建自定义配置
        auto customConfig = RotationConfigBuilder()
            .Strategy(LogRotationStrategy::SizeAndTime)
            .MaxSize(75)
            .MaxBackups(15)
            .TimeInterval(TimeRotationInterval::Daily)
            .EnableCompression(true, 7)
            .AsyncMode(true)
            .Archive(L"logs/custom_archive")
            .AutoCleanup(60)
            .DiskThreshold(500)
            .FilePattern(L"{basename}_custom_{timestamp}.{extension}")
            .TimestampFormat(L"%Y%m%d_%H%M")
            .Build();
            
        std::wcout << L"自定义配置创建完成:\n";
        std::wcout << L"  策略: 复合策略\n";
        std::wcout << L"  大小阈值: " << customConfig.maxFileSizeMB << L" MB\n";
        std::wcout << L"  时间间隔: 每天\n";
        std::wcout << L"  备份数: " << customConfig.maxBackupFiles << L"\n";
        std::wcout << L"  压缩级别: " << customConfig.compressionLevel << L"\n";
        std::wcout << L"  归档路径: " << customConfig.archivePath << L"\n";
        std::wcout << L"  清理周期: " << customConfig.cleanupOlderThanDays << L" 天\n";
        std::wcout << L"  文件模式: " << customConfig.rotatedFilePattern << L"\n";
        
        // 创建管理器并测试
        auto compressor = std::make_shared<LogCompressor>();
        auto manager = RotationManagerFactory::CreateAsyncRotationManager(customConfig, compressor);
        
        manager->SetRotationCallback([](const RotationResult& result) {
            std::wcout << L"[自定义配置] 轮转结果: " 
                      << (result.success ? L"成功" : L"失败") << L"\n";
        });
        
        manager->Start();
        
        // 测试轮转
        auto trigger = manager->CheckRotationNeeded(L"logs/custom.log", 70 * 1024 * 1024);
        if (trigger.sizeExceeded) {
            manager->ForceRotation(L"logs/custom.log", L"Custom config test");
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        manager->Stop();
        
    } catch (const std::exception& e) {
        std::cerr << "Config builder demonstration error: " << e.what() << std::endl;
    }
    
    std::wcout << L"\n";
}

/**
 * @brief 主函数
 */
int main() {
    std::wcout << L"LightLogWriteImpl 配置示例演示\n";
    std::wcout << L"================================\n\n";
    
    try {
        // 创建必要的目录
        system("mkdir logs 2>nul");
        system("mkdir logs\\archive 2>nul");
        system("mkdir logs\\compliance_archive 2>nul");
        system("mkdir logs\\custom_archive 2>nul");
        
        // 演示预设配置
        DemonstrateConfigPresets();
        
        // 演示配置构建器
        DemonstrateConfigBuilder();
        
        std::wcout << L"配置示例演示完成！\n";
        std::wcout << L"请查看生成的日志文件和配置效果。\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Main configuration example error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}