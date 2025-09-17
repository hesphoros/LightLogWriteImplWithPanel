#include "log/LightLogWriteImpl.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>

// 辅助函数：获取当前时间字符串
std::wstring GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm_buf;
    localtime_s(&tm_buf, &time_t);
    
    wchar_t buffer[32];
    wcsftime(buffer, sizeof(buffer) / sizeof(wchar_t), L"%H:%M:%S", &tm_buf);
    
    return std::wstring(buffer) + L"." + std::to_wstring(ms.count());
}

// 示例1：基本使用
void BasicUsageExample() {
    std::wcout << L"\n=== 基本使用示例 ===" << std::endl;
    
    auto logger = std::make_unique<LightLogWrite_Impl>();
    
    // 设置日志文件
    logger->SetLastingsLogs(L"logs", L"basic_example");
    
    // 写入基本日志
    logger->WriteLogInfo(L"应用程序启动");
    logger->WriteLogInfo(L"初始化完成");
    logger->WriteLogInfo(L"开始处理业务逻辑");
    
    // 等待处理完成
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::wcout << L"基本使用示例完成" << std::endl;
}

// 示例2：自定义轮转配置
void CustomRotationConfigExample() {
    std::wcout << L"\n=== 自定义轮转配置示例 ===" << std::endl;
    
    auto logger = std::make_unique<LightLogWrite_Impl>();
    
    // 设置日志文件
    logger->SetLastingsLogs(L"example_logs", L"demo_log");
    
    // 配置轮转设置
    LogRotationConfig config;
    config.strategy = LogRotationStrategy::Size;
    config.maxFileSizeMB = 1;  // 1MB 文件大小轮转
    config.maxArchiveFiles = 5;
    config.enableCompression = true;
    
    logger->SetLogRotationConfig(config);
    
    std::wcout << L"开始生成大量日志数据..." << std::endl;
    
    // 生成足够的数据触发轮转
    for (int i = 0; i < 5000; ++i) {
        std::wstring msg = L"测试消息 #" + std::to_wstring(i) + 
                          L" - 时间: " + GetCurrentTimeString() +
                          L" - 这是一条较长的日志消息用于快速填充文件以触发轮转机制，包含更多内容以便快速达到文件大小限制";
        logger->WriteLogInfo(msg);
        
        if (i % 500 == 0) {
            std::wcout << L"已生成 " << i << L" 条日志..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    // 等待处理完成
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::wcout << L"自定义配置示例完成，请检查 example_logs 目录" << std::endl;
}

// 示例3：手动轮转
void ManualRotationExample() {
    std::wcout << L"\n=== 手动轮转示例 ===" << std::endl;
    
    auto logger = std::make_unique<LightLogWrite_Impl>();
    logger->SetLastingsLogs(L"manual_rotation_logs", L"batch_log");
    
    // 启用压缩轮转
    LogRotationConfig config;
    config.strategy = LogRotationStrategy::Size;
    config.maxFileSizeMB = 10;  // 较大的文件大小，主要依靠手动轮转
    config.enableCompression = true;
    logger->SetLogRotationConfig(config);
    
    // 模拟批处理任务
    for (int batch = 1; batch <= 3; ++batch) {
        logger->WriteLogInfo(L"开始处理批次 " + std::to_wstring(batch));
        
        // 每个批次生成一些日志
        for (int i = 0; i < 1000; ++i) {
            std::wstring msg = L"[BATCH-" + std::to_wstring(batch) + L"] " +
                              L"处理项目 " + std::to_wstring(i) + 
                              L" - 时间: " + GetCurrentTimeString() +
                              L" - 详细处理信息和结果数据";
            logger->WriteLogInfo(msg);
        }
        
        logger->WriteLogInfo(L"批次 " + std::to_wstring(batch) + L" 完成");
        
        // 批次完成后手动轮转
        std::wcout << L"触发批次 " << batch << L" 的手动轮转..." << std::endl;
        logger->ForceLogRotation();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    logger->WriteLogInfo(L"所有批处理任务完成");
    std::wcout << L"手动轮转示例完成，请检查 manual_rotation_logs 目录" << std::endl;
}

// 示例4：多线程并发写入
void MultiThreadExample() {
    std::wcout << L"\n=== 多线程并发示例 ===" << std::endl;
    
    auto logger = std::make_shared<LightLogWrite_Impl>();
    logger->SetLastingsLogs(L"multithread_logs", L"concurrent_log");
    
    // 配置较小的文件大小以观察轮转
    LogRotationConfig config;
    config.strategy = LogRotationStrategy::Size;
    config.maxFileSizeMB = 2;
    config.enableCompression = true;
    logger->SetLogRotationConfig(config);
    
    const int numThreads = 4;
    const int messagesPerThread = 800;
    std::vector<std::thread> threads;
    
    std::wcout << L"启动 " << numThreads << L" 个并发线程..." << std::endl;
    
    // 启动工作线程
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([logger, t, messagesPerThread]() {
            for (int i = 0; i < messagesPerThread; ++i) {
                std::wstring msg = L"[THREAD-" + std::to_wstring(t) + L"] " +
                                  L"消息-" + std::to_wstring(i) + 
                                  L" 时间-" + GetCurrentTimeString() +
                                  L" 额外数据用于测试并发写入的稳定性和线程安全性";
                
                // 使用不同的日志级别
                switch (i % 4) {
                    case 0: logger->WriteLogInfo(msg); break;
                    case 1: logger->WriteLogDebug(msg); break;
                    case 2: logger->WriteLogWarning(msg); break;
                    case 3: logger->WriteLogError(msg); break;
                }
                
                if (i % 100 == 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(500));
                }
            }
        });
    }
    
    // 监控线程
    std::thread monitorThread([logger]() {
        for (int i = 0; i < 15; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            if (i % 5 == 0 && i > 0) {
                logger->WriteLogInfo(L"监控检查 - 时间: " + GetCurrentTimeString());
                std::wcout << L"当前日志文件大小: " << logger->GetCurrentLogFileSize() << L" bytes" << std::endl;
            }
        }
    });
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    monitorThread.join();
    
    logger->WriteLogInfo(L"多线程测试完成");
    std::wcout << L"多线程示例完成，请检查 multithread_logs 目录" << std::endl;
}

// 示例5：压缩效果测试
void CompressionTestExample() {
    std::wcout << L"\n=== 压缩效果测试示例 ===" << std::endl;
    
    auto logger = std::make_unique<LightLogWrite_Impl>();
    logger->SetLastingsLogs(L"compression_test_logs", L"compression_test");
    
    // 配置小文件大小以快速触发压缩
    LogRotationConfig config;
    config.strategy = LogRotationStrategy::Size;
    config.maxFileSizeMB = 1;  // 1MB
    config.enableCompression = true;
    config.maxArchiveFiles = 20;  // 保留更多归档文件用于测试
    logger->SetLogRotationConfig(config);
    
    std::wcout << L"生成包含重复内容的日志数据以测试压缩效果..." << std::endl;
    
    // 生成包含重复模式的数据（更易压缩）
    for (int round = 1; round <= 5; ++round) {
        logger->WriteLogInfo(L"=== 压缩测试轮次 " + std::to_wstring(round) + L" 开始 ===");
        
        for (int i = 0; i < 1500; ++i) {
            std::wstring msg = L"[COMPRESSION-TEST-R" + std::to_wstring(round) + L"] " +
                              L"重复模式消息 " + std::to_wstring(i) + 
                              L" - 固定内容用于测试ZIP压缩效果 - " +
                              L"ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 - " +
                              L"这是一个包含大量重复数据的长消息，" +
                              L"用于验证miniz压缩库的压缩效果和性能表现 - " +
                              L"时间戳: " + GetCurrentTimeString();
            
            logger->WriteLogInfo(msg);
        }
        
        logger->WriteLogInfo(L"=== 压缩测试轮次 " + std::to_wstring(round) + L" 结束 ===");
        
        // 手动触发轮转以生成压缩文件
        logger->ForceLogRotation();
        
        std::wcout << L"轮次 " << round << L" 完成，已触发轮转" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // 等待处理完成
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 最终轮转
    logger->WriteLogInfo(L"压缩测试完成，执行最终轮转");
    logger->ForceLogRotation();
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::wcout << L"压缩测试完成！" << std::endl;
    std::wcout << L"请检查 compression_test_logs 目录中的文件：" << std::endl;
    std::wcout << L"- .log 文件：当前活动日志" << std::endl;
    std::wcout << L"- .zip 文件：压缩的归档日志" << std::endl;
    std::wcout << L"比较 .log 和 .zip 文件大小来查看压缩效果" << std::endl;
}

// 示例6：时间轮转测试
void TimeBasedRotationExample() {
    std::wcout << L"\n=== 时间轮转测试示例 ===" << std::endl;
    
    auto logger = std::make_unique<LightLogWrite_Impl>();
    logger->SetLastingsLogs(L"time_rotation_logs", L"time_test");
    
    // 注意：实际的时间轮转需要较长时间，这里主要演示配置
    LogRotationConfig config;
    config.strategy = LogRotationStrategy::Time;
    config.timeInterval = TimeRotationInterval::Hourly;  // 按小时轮转
    config.enableCompression = true;
    logger->SetLogRotationConfig(config);
    
    logger->WriteLogInfo(L"时间轮转测试开始");
    logger->WriteLogInfo(L"配置：按小时轮转，启用压缩");
    
    // 生成一些日志数据
    for (int i = 0; i < 100; ++i) {
        std::wstring msg = L"时间轮转测试消息 " + std::to_wstring(i) + 
                          L" - 当前时间: " + GetCurrentTimeString();
        logger->WriteLogInfo(msg);
        
        if (i % 10 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    logger->WriteLogInfo(L"时间轮转测试完成（实际轮转需要等待设定的时间间隔）");
    
    std::wcout << L"时间轮转示例完成" << std::endl;
    std::wcout << L"注意：实际的时间轮转需要等待设定的时间间隔才会发生" << std::endl;
}

// 显示配置信息
void ShowConfigurationExample() {
    std::wcout << L"\n=== 配置信息示例 ===" << std::endl;
    
    auto logger = std::make_unique<LightLogWrite_Impl>();
    logger->SetLastingsLogs(L"config_test_logs", L"config_test");
    
    // 设置配置
    LogRotationConfig config;
    config.strategy = LogRotationStrategy::Size;
    config.maxFileSizeMB = 5;
    config.maxArchiveFiles = 10;
    config.enableCompression = true;
    logger->SetLogRotationConfig(config);
    
    // 获取并显示配置
    auto currentConfig = logger->GetLogRotationConfig();
    
    std::wcout << L"当前轮转配置：" << std::endl;
    std::wcout << L"  策略: " << (currentConfig.strategy == LogRotationStrategy::Size ? L"按文件大小" : 
                                  currentConfig.strategy == LogRotationStrategy::Time ? L"按时间" : L"无") << std::endl;
    std::wcout << L"  最大文件大小: " << currentConfig.maxFileSizeMB << L" MB" << std::endl;
    std::wcout << L"  最大归档文件数: " << currentConfig.maxArchiveFiles << std::endl;
    std::wcout << L"  启用压缩: " << (currentConfig.enableCompression ? L"是" : L"否") << std::endl;
    
    logger->WriteLogInfo(L"配置信息已显示");
    
    std::wcout << L"配置信息示例完成" << std::endl;
}

int main() {
    std::wcout << L"LightLogWrite_Impl 轮转压缩机制示例程序" << std::endl;
    std::wcout << L"=============================================" << std::endl;
    
    try {
        // 运行所有示例
        BasicUsageExample();
        CustomRotationConfigExample();
        ManualRotationExample();
        MultiThreadExample();
        CompressionTestExample();
        TimeBasedRotationExample();
        ShowConfigurationExample();
        
        std::wcout << L"\n🎉 所有示例执行完成！" << std::endl;
        std::wcout << L"\n📁 请检查以下目录中生成的日志文件：" << std::endl;
        std::wcout << L"   ├── logs/ (基本示例)" << std::endl;
        std::wcout << L"   ├── example_logs/ (自定义配置)" << std::endl;
        std::wcout << L"   ├── manual_rotation_logs/ (手动轮转)" << std::endl;
        std::wcout << L"   ├── multithread_logs/ (多线程测试)" << std::endl;
        std::wcout << L"   ├── compression_test_logs/ (压缩测试)" << std::endl;
        std::wcout << L"   ├── time_rotation_logs/ (时间轮转)" << std::endl;
        std::wcout << L"   └── config_test_logs/ (配置测试)" << std::endl;
        
        std::wcout << L"\n📋 观察要点：" << std::endl;
        std::wcout << L"   ✓ .log 文件：当前正在写入的活动日志" << std::endl;
        std::wcout << L"   ✓ .zip 文件：已轮转并压缩的归档日志" << std::endl;
        std::wcout << L"   ✓ 比较文件大小可以看到显著的压缩效果" << std::endl;
        std::wcout << L"   ✓ 压缩率通常可达 90-98%，大幅节省存储空间" << std::endl;
        
    }
    catch (const std::exception& e) {
        std::wcerr << L"❌ 示例程序执行出错: " << e.what() << std::endl;
        return 1;
    }
    
    std::wcout << L"\n按任意键退出..." << std::endl;
    std::cin.get();
    
    return 0;
}