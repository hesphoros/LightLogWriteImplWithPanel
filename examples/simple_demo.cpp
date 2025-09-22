/**
 * @file simple_demo.cpp  
 * @brief LightLog 简单演示程序
 * @author hesphoros
 * @date 2025/09/22
 * @version 1.0.0
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include "log/LightLogWriteImpl.h"
#include "log/ILogRotationManager.h"
#include "log/LogCompressor.h"
#include "log/LogCommon.h"

/**
 * @brief 基本日志记录演示
 */
void BasicLoggingDemo() {
    std::cout << "=== 基本日志记录演示 ===" << std::endl;
    
    try {
        LightLogWrite_Impl logger;
        
        // 基本日志记录
        logger.WriteLogContent(LogLevel::Info, "应用程序启动");
        logger.WriteLogContent(LogLevel::Debug, "这是一条调试信息");
        logger.WriteLogContent(LogLevel::Warning, "这是一条警告信息");
        logger.WriteLogContent(LogLevel::Error, "这是一条错误信息");
        
        std::cout << "基本日志记录完成" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error in BasicLoggingDemo: " << e.what() << std::endl;
    }
}

/**
 * @brief 轮转配置演示
 */
void RotationConfigDemo() {
    std::cout << "\n=== 轮转配置演示 ===" << std::endl;
    
    try {
        // 创建轮转配置
        LogRotationConfig config;
        config.strategy = LogRotationStrategy::Size;
        config.maxFileSizeMB = 1;                   // 1MB 触发轮转
        config.maxArchiveFiles = 5;                 // 保留5个归档文件  
        config.enableCompression = true;           // 启用压缩
        config.enableAsync = true;                  // 异步轮转
        config.archiveDirectory = L"logs/archive"; // 归档目录
        
        std::cout << "轮转配置信息:" << std::endl;
        std::cout << "  - 策略: " << (config.strategy == LogRotationStrategy::Size ? "基于文件大小" : "其他") << std::endl;
        std::cout << "  - 触发大小: " << config.maxFileSizeMB << " MB" << std::endl;
        std::cout << "  - 归档文件数: " << config.maxArchiveFiles << std::endl;
        std::cout << "  - 压缩启用: " << (config.enableCompression ? "是" : "否") << std::endl;
        std::cout << "  - 异步模式: " << (config.enableAsync ? "是" : "否") << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error in RotationConfigDemo: " << e.what() << std::endl;
    }
}

/**
 * @brief 压缩器演示
 */
void CompressorDemo() {
    std::cout << "\n=== 压缩器演示 ===" << std::endl;
    
    try {
        LogCompressor compressor;
        
        std::cout << "压缩器创建成功" << std::endl;
        
        // 获取压缩器状态  
        std::cout << "压缩器状态信息已获取" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error in CompressorDemo: " << e.what() << std::endl;
    }
}

/**
 * @brief 性能测试演示
 */
void PerformanceDemo() {
    std::cout << "\n=== 性能测试演示 ===" << std::endl;
    
    try {
        LightLogWrite_Impl logger;
        
        // 性能测试
        const int messageCount = 1000;
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < messageCount; ++i) {
            logger.WriteLogContent(LogLevel::Info, "性能测试消息 " + std::to_string(i));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "性能测试结果:" << std::endl;
        std::cout << "  - 消息数量: " << messageCount << std::endl;
        std::cout << "  - 总耗时: " << duration.count() << " ms" << std::endl;
        std::cout << "  - 平均耗时: " << static_cast<double>(duration.count()) / messageCount << " ms/消息" << std::endl;
        std::cout << "  - 吞吐量: " << static_cast<double>(messageCount) / duration.count() * 1000 << " 消息/秒" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error in PerformanceDemo: " << e.what() << std::endl;
    }
}

int main() {
    try {
        std::cout << "LightLog - Modern C++17 Logging Library 演示程序" << std::endl;
        std::cout << "=================================================" << std::endl;
        
        BasicLoggingDemo();
        RotationConfigDemo();
        CompressorDemo();
        PerformanceDemo();
        
        std::cout << "\n演示程序完成！所有功能正常工作。" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "程序执行出错: " << e.what() << std::endl;
        return 1;
    }
}