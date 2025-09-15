/**
 * @file rotation_strategy_examples.cpp
 * @brief 日志轮转策略使用示例
 * @author hesphoros
 * @date 2025/09/16
 * @version 1.0.0
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include "../include/log/LightLogWriteImpl.h"
#include "../include/log/ILogRotationManager.h"
#include "../include/log/RotationManagerFactory.h"
#include "../include/log/LogCompressor.h"

/**
 * @brief 示例1：基于文件大小的轮转策略
 * 适用场景：高频日志写入，需要严格控制单个日志文件大小
 */
void ExampleSizeBasedRotation() {
    std::wcout << L"=== 示例1：基于文件大小的轮转策略 ===\n";
    
    try {
        // 配置基于大小的轮转
        LogRotationConfig config;
        config.strategy = LogRotationStrategy::Size;
        config.enabled = true;
        config.maxFileSizeMB = 10;              // 10MB 触发轮转
        config.maxBackupFiles = 5;              // 保留5个备份文件
        config.compressionEnabled = true;       // 启用压缩
        config.asyncRotation = true;            // 异步轮转，不阻塞主线程
        config.diskSpaceThresholdMB = 100;      // 磁盘空间低于100MB时停止轮转
        
        // 创建压缩器
        auto compressor = std::make_shared<LogCompressor>();
        
        // 创建轮转管理器
        auto rotationManager = RotationManagerFactory::CreateAsyncRotationManager(config, compressor);
        
        // 设置轮转回调
        rotationManager->SetRotationCallback([](const RotationResult& result) {
            if (result.success) {
                std::wcout << L"[回调] 轮转成功: " << result.newFileName 
                          << L" (原因: " << result.trigger.reason << L")\n";
            } else {
                std::wcout << L"[回调] 轮转失败: " << result.errorMessage << L"\n";
            }
        });
        
        // 启动轮转管理器
        rotationManager->Start();
        
        std::wcout << L"配置信息:\n";
        std::wcout << L"  - 策略: 基于文件大小\n";
        std::wcout << L"  - 触发大小: " << config.maxFileSizeMB << L" MB\n";
        std::wcout << L"  - 备份文件数: " << config.maxBackupFiles << L"\n";
        std::wcout << L"  - 压缩: " << (config.compressionEnabled ? L"启用" : L"禁用") << L"\n";
        std::wcout << L"  - 异步模式: " << (config.asyncRotation ? L"启用" : L"禁用") << L"\n";
        
        // 模拟日志写入
        std::wstring logFile = L"logs/size_based_example.log";
        for (int i = 0; i < 100; ++i) {
            // 检查是否需要轮转
            if (rotationManager->CheckRotationNeeded(logFile, 1024 * 1024 * (i / 10 + 1)).sizeExceeded) {
                rotationManager->ForceRotation(logFile, L"Size threshold reached");
            }
            
            // 模拟写入延迟
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (i % 20 == 0) {
                std::wcout << L"已写入 " << i << L" 条日志记录\n";
            }
        }
        
        // 停止轮转管理器
        rotationManager->Stop();
        
        // 获取统计信息
        auto stats = rotationManager->GetStatistics();
        std::wcout << L"轮转统计:\n";
        std::wcout << L"  - 总轮转次数: " << stats.totalRotations << L"\n";
        std::wcout << L"  - 成功次数: " << stats.successfulRotations << L"\n";
        std::wcout << L"  - 失败次数: " << stats.failedRotations << L"\n";
        std::wcout << L"  - 平均轮转时间: " << stats.averageRotationTimeMs << L" ms\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Size-based rotation example error: " << e.what() << std::endl;
    }
    
    std::wcout << L"\n";
}

/**
 * @brief 示例2：基于时间间隔的轮转策略
 * 适用场景：定期归档日志，如每天或每小时轮转一次
 */
void ExampleTimeBasedRotation() {
    std::wcout << L"=== 示例2：基于时间间隔的轮转策略 ===\n";
    
    try {
        // 配置基于时间的轮转
        LogRotationConfig config;
        config.strategy = LogRotationStrategy::Time;
        config.enabled = true;
        config.timeInterval = TimeRotationInterval::Hourly;  // 每小时轮转
        config.maxBackupFiles = 24;             // 保留24小时的备份
        config.compressionEnabled = true;       // 启用压缩
        config.asyncRotation = false;           // 同步轮转，确保时间精确性
        config.archiveOldFiles = true;          // 归档旧文件
        config.archivePath = L"logs/archive";   // 归档目录
        
        // 创建压缩器
        auto compressor = std::make_shared<LogCompressor>();
        
        // 创建轮转管理器
        auto rotationManager = RotationManagerFactory::CreateAsyncRotationManager(config, compressor);
        
        // 设置轮转回调
        rotationManager->SetRotationCallback([](const RotationResult& result) {
            if (result.success) {
                std::wcout << L"[回调] 定时轮转完成: " << result.newFileName << L"\n";
                std::wcout << L"  轮转时间: " << result.rotationTimeMs << L" ms\n";
            }
        });
        
        // 启动轮转管理器
        rotationManager->Start();
        
        std::wcout << L"配置信息:\n";
        std::wcout << L"  - 策略: 基于时间间隔\n";
        std::wcout << L"  - 轮转间隔: 每小时\n";
        std::wcout << L"  - 备份文件数: " << config.maxBackupFiles << L"\n";
        std::wcout << L"  - 归档: " << (config.archiveOldFiles ? L"启用" : L"禁用") << L"\n";
        std::wcout << L"  - 归档路径: " << config.archivePath << L"\n";
        
        // 模拟长时间运行
        std::wstring logFile = L"logs/time_based_example.log";
        auto startTime = std::chrono::system_clock::now();
        
        for (int i = 0; i < 10; ++i) {
            // 检查时间触发的轮转
            auto trigger = rotationManager->CheckRotationNeeded(logFile, 1024);
            if (trigger.timeReached) {
                std::wcout << L"时间间隔到达，执行轮转\n";
                rotationManager->ForceRotation(logFile, L"Time interval reached");
            }
            
            // 模拟时间流逝
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::wcout << L"运行时间: " << (i + 1) << L" 秒\n";
        }
        
        rotationManager->Stop();
        
    } catch (const std::exception& e) {
        std::cerr << "Time-based rotation example error: " << e.what() << std::endl;
    }
    
    std::wcout << L"\n";
}

/**
 * @brief 示例3：复合轮转策略（大小+时间）
 * 适用场景：生产环境，既要控制文件大小又要定期归档
 */
void ExampleCompositeRotation() {
    std::wcout << L"=== 示例3：复合轮转策略（大小+时间） ===\n";
    
    try {
        // 配置复合轮转策略
        LogRotationConfig config;
        config.strategy = LogRotationStrategy::SizeAndTime;
        config.enabled = true;
        config.maxFileSizeMB = 50;              // 50MB 触发大小轮转
        config.timeInterval = TimeRotationInterval::Daily;  // 每天定时轮转
        config.maxBackupFiles = 30;             // 保留30天的备份
        config.compressionEnabled = true;       // 启用压缩
        config.asyncRotation = true;            // 异步轮转
        config.archiveOldFiles = true;          // 归档旧文件
        config.archivePath = L"logs/archive";   // 归档目录
        config.diskSpaceThresholdMB = 1000;     // 磁盘空间阈值 1GB
        config.autoCleanup = true;              // 自动清理
        config.cleanupOlderThanDays = 30;       // 清理30天前的日志
        
        // 创建高性能压缩器
        auto compressor = std::make_shared<LogCompressor>();
        
        // 创建轮转管理器
        auto rotationManager = RotationManagerFactory::CreateAsyncRotationManager(config, compressor);
        
        // 设置详细的轮转回调
        rotationManager->SetRotationCallback([](const RotationResult& result) {
            if (result.success) {
                std::wcout << L"[复合轮转] 成功完成:\n";
                std::wcout << L"  原文件: " << result.originalFileName << L"\n";
                std::wcout << L"  新文件: " << result.newFileName << L"\n";
                std::wcout << L"  触发原因: " << result.trigger.reason << L"\n";
                std::wcout << L"  文件大小: " << (result.trigger.currentFileSize / 1024 / 1024) << L" MB\n";
                std::wcout << L"  耗时: " << result.rotationTimeMs << L" ms\n";
                
                if (result.compressionResult.compressed) {
                    auto ratio = (double)result.compressionResult.compressedSize / result.compressionResult.originalSize * 100;
                    std::wcout << L"  压缩比: " << std::fixed << std::setprecision(1) << ratio << L"%\n";
                }
            } else {
                std::wcout << L"[复合轮转] 失败: " << result.errorMessage << L"\n";
            }
        });
        
        // 启动轮转管理器
        rotationManager->Start();
        
        std::wcout << L"配置信息:\n";
        std::wcout << L"  - 策略: 复合策略（大小+时间）\n";
        std::wcout << L"  - 大小阈值: " << config.maxFileSizeMB << L" MB\n";
        std::wcout << L"  - 时间间隔: 每天\n";
        std::wcout << L"  - 备份保留: " << config.maxBackupFiles << L" 个\n";
        std::wcout << L"  - 自动清理: " << config.cleanupOlderThanDays << L" 天\n";
        std::wcout << L"  - 磁盘阈值: " << config.diskSpaceThresholdMB << L" MB\n";
        
        // 模拟生产环境的日志写入
        std::wstring logFile = L"logs/composite_example.log";
        
        for (int day = 0; day < 3; ++day) {
            std::wcout << L"\n--- 第 " << (day + 1) << L" 天 ---\n";
            
            for (int hour = 0; hour < 24; hour += 6) {  // 每6小时检查一次
                size_t simulatedFileSize = 1024 * 1024 * (10 + hour * 2);  // 模拟文件增长
                
                auto trigger = rotationManager->CheckRotationNeeded(logFile, simulatedFileSize);
                
                if (trigger.sizeExceeded || trigger.timeReached) {
                    std::wcout << L"触发轮转 (时间: " << hour << L":00, 大小: " 
                              << (simulatedFileSize / 1024 / 1024) << L" MB)\n";
                    
                    std::wstring reason = trigger.sizeExceeded ? L"Size exceeded" : L"Time reached";
                    rotationManager->ForceRotation(logFile, reason);
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
        
        rotationManager->Stop();
        
        // 显示最终统计
        auto stats = rotationManager->GetStatistics();
        std::wcout << L"\n最终统计:\n";
        std::wcout << L"  - 总轮转次数: " << stats.totalRotations << L"\n";
        std::wcout << L"  - 成功次数: " << stats.successfulRotations << L"\n";
        std::wcout << L"  - 大小触发: " << stats.sizeTriggeredRotations << L"\n";
        std::wcout << L"  - 时间触发: " << stats.timeTriggeredRotations << L"\n";
        std::wcout << L"  - 平均轮转时间: " << stats.averageRotationTimeMs << L" ms\n";
        std::wcout << L"  - 总压缩节省: " << (stats.totalSpaceSavedMB) << L" MB\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Composite rotation example error: " << e.what() << std::endl;
    }
    
    std::wcout << L"\n";
}

/**
 * @brief 示例4：手动轮转和高级配置
 * 适用场景：需要精确控制轮转时机的应用
 */
void ExampleManualRotation() {
    std::wcout << L"=== 示例4：手动轮转和高级配置 ===\n";
    
    try {
        // 配置手动轮转
        LogRotationConfig config;
        config.strategy = LogRotationStrategy::None;  // 禁用自动轮转
        config.enabled = true;
        config.maxBackupFiles = 10;
        config.compressionEnabled = true;
        config.asyncRotation = false;           // 同步轮转，便于控制
        config.preserveFileAttributes = true;   // 保留文件属性
        config.secureDelete = true;             // 安全删除
        
        // 设置文件名模式
        config.rotatedFilePattern = L"{basename}_{timestamp}_{index}.{extension}";
        config.timestampFormat = L"%Y%m%d_%H%M%S";
        
        auto compressor = std::make_shared<LogCompressor>();
        auto rotationManager = RotationManagerFactory::CreateAsyncRotationManager(config, compressor);
        
        // 设置回调监控手动轮转
        rotationManager->SetRotationCallback([](const RotationResult& result) {
            std::wcout << L"[手动轮转] " << (result.success ? L"成功" : L"失败") << L": "
                      << result.newFileName << L"\n";
        });
        
        rotationManager->Start();
        
        std::wcout << L"手动轮转演示:\n";
        std::wstring logFile = L"logs/manual_example.log";
        
        // 场景1：关键事件触发轮转
        std::wcout << L"场景1：关键事件触发轮转\n";
        rotationManager->ForceRotation(logFile, L"Critical error occurred");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 场景2：定期备份
        std::wcout << L"场景2：定期备份\n";
        rotationManager->ForceRotation(logFile, L"Daily backup");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 场景3：维护窗口轮转
        std::wcout << L"场景3：维护窗口轮转\n";
        rotationManager->ForceRotation(logFile, L"Maintenance window");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 获取轮转历史
        auto stats = rotationManager->GetStatistics();
        std::wcout << L"手动轮转统计:\n";
        std::wcout << L"  - 手动轮转次数: " << stats.manualRotations << L"\n";
        std::wcout << L"  - 成功率: " << (stats.successfulRotations * 100.0 / stats.totalRotations) << L"%\n";
        
        rotationManager->Stop();
        
    } catch (const std::exception& e) {
        std::cerr << "Manual rotation example error: " << e.what() << std::endl;
    }
    
    std::wcout << L"\n";
}

/**
 * @brief 示例5：多文件轮转管理
 * 适用场景：管理多个日志文件的轮转
 */
void ExampleMultiFileRotation() {
    std::wcout << L"=== 示例5：多文件轮转管理 ===\n";
    
    try {
        // 为不同类型的日志配置不同的轮转策略
        struct LogFileConfig {
            std::wstring fileName;
            LogRotationConfig config;
            std::unique_ptr<ILogRotationManager> manager;
        };
        
        std::vector<LogFileConfig> logConfigs;
        auto compressor = std::make_shared<LogCompressor>();
        
        // 错误日志：小文件，长期保留
        LogRotationConfig errorConfig;
        errorConfig.strategy = LogRotationStrategy::Size;
        errorConfig.maxFileSizeMB = 5;
        errorConfig.maxBackupFiles = 50;
        errorConfig.compressionEnabled = true;
        
        // 访问日志：大文件，短期保留
        LogRotationConfig accessConfig;
        accessConfig.strategy = LogRotationStrategy::SizeAndTime;
        accessConfig.maxFileSizeMB = 100;
        accessConfig.timeInterval = TimeRotationInterval::Daily;
        accessConfig.maxBackupFiles = 7;
        accessConfig.compressionEnabled = true;
        
        // 调试日志：频繁轮转，不压缩
        LogRotationConfig debugConfig;
        debugConfig.strategy = LogRotationStrategy::Size;
        debugConfig.maxFileSizeMB = 20;
        debugConfig.maxBackupFiles = 3;
        debugConfig.compressionEnabled = false;
        
        // 创建管理器
        std::vector<std::pair<std::wstring, std::unique_ptr<ILogRotationManager>>> managers;
        
        managers.emplace_back(L"logs/error.log", 
            RotationManagerFactory::CreateAsyncRotationManager(errorConfig, compressor));
        managers.emplace_back(L"logs/access.log", 
            RotationManagerFactory::CreateAsyncRotationManager(accessConfig, compressor));
        managers.emplace_back(L"logs/debug.log", 
            RotationManagerFactory::CreateAsyncRotationManager(debugConfig, compressor));
        
        // 启动所有管理器
        for (auto& [fileName, manager] : managers) {
            manager->SetRotationCallback([fileName](const RotationResult& result) {
                std::wcout << L"[" << fileName << L"] 轮转" 
                          << (result.success ? L"成功" : L"失败") << L"\n";
            });
            manager->Start();
        }
        
        std::wcout << L"多文件轮转配置:\n";
        std::wcout << L"  - error.log: 5MB, 50个备份, 压缩\n";
        std::wcout << L"  - access.log: 100MB/天, 7个备份, 压缩\n";
        std::wcout << L"  - debug.log: 20MB, 3个备份, 不压缩\n";
        
        // 模拟不同文件的写入模式
        for (int i = 0; i < 20; ++i) {
            // 错误日志：偶尔写入大量数据
            if (i % 5 == 0) {
                auto trigger = managers[0].second->CheckRotationNeeded(managers[0].first, 1024 * 1024 * 3);
                if (trigger.sizeExceeded) {
                    managers[0].second->ForceRotation(managers[0].first, L"Error burst");
                }
            }
            
            // 访问日志：持续写入
            auto trigger = managers[1].second->CheckRotationNeeded(managers[1].first, 1024 * 1024 * (20 + i));
            if (trigger.sizeExceeded || trigger.timeReached) {
                managers[1].second->ForceRotation(managers[1].first, L"Access log rotation");
            }
            
            // 调试日志：频繁写入
            trigger = managers[2].second->CheckRotationNeeded(managers[2].first, 1024 * 1024 * (i * 2));
            if (trigger.sizeExceeded) {
                managers[2].second->ForceRotation(managers[2].first, L"Debug log rotation");
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        
        // 停止所有管理器并收集统计
        std::wcout << L"\n各文件轮转统计:\n";
        for (auto& [fileName, manager] : managers) {
            manager->Stop();
            auto stats = manager->GetStatistics();
            std::wcout << L"  " << fileName << L": " << stats.totalRotations 
                      << L" 次轮转, " << stats.totalSpaceSavedMB << L" MB 节省\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Multi-file rotation example error: " << e.what() << std::endl;
    }
    
    std::wcout << L"\n";
}

/**
 * @brief 主函数：运行所有轮转策略示例
 */
int main() {
    std::wcout << L"LightLogWriteImpl 轮转策略使用示例\n";
    std::wcout << L"=====================================\n\n";
    
    try {
        // 创建日志目录
        system("mkdir logs 2>nul");
        system("mkdir logs\\archive 2>nul");
        
        // 运行各种轮转策略示例
        ExampleSizeBasedRotation();
        ExampleTimeBasedRotation();
        ExampleCompositeRotation();
        ExampleManualRotation();
        ExampleMultiFileRotation();
        
        std::wcout << L"所有示例运行完毕！\n";
        std::wcout << L"请查看 logs/ 目录中生成的日志文件和轮转结果。\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Main example error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}