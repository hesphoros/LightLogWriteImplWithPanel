/**
 * @file rotation_demo.cpp
 * @brief 日志轮转系统使用示例
 * @details 展示如何使用新的模块化轮转系统
 */

#include <iostream>
#include <thread>
#include <chrono>
#include "../include/log/LightLogWriteImpl.h"

void DemonstrateRotationSystem() {
    std::wcout << L"=== 日志轮转系统演示 ===" << std::endl;
    
    // 创建日志实例
    LightLogWrite_Impl logger(10000);  // 队列大小10000
    
    // 设置日志文件
    logger.SetLastingsLogs(L"./logs", L"rotation_demo");
    
    // 配置轮转系统
    LogRotationConfig config;
    config.strategy = LogRotationStrategy::Size;          // 按大小轮转
    config.maxFileSizeMB = 1;                            // 1MB触发轮转
    config.maxArchiveFiles = 5;                          // 保留5个归档文件
    config.enableCompression = true;                     // 启用压缩
    config.enableAsync = true;                           // 启用异步轮转
    config.enablePreCheck = true;                        // 启用预检查
    config.enableTransaction = true;                     // 启用事务机制
    
    logger.SetLogRotationConfig(config);
    
    std::wcout << L"✅ 轮转系统配置完成" << std::endl;
    std::wcout << L"   - 策略: 按大小轮转 (1MB)" << std::endl;
    std::wcout << L"   - 异步处理: 启用" << std::endl;
    std::wcout << L"   - 预检查: 启用" << std::endl;
    std::wcout << L"   - 事务机制: 启用" << std::endl;
    
    // 写入大量日志以触发轮转
    std::wcout << L"📝 开始写入日志数据..." << std::endl;
    
    for (int i = 0; i < 1000; ++i) {
        std::wstring message = L"这是测试日志消息 #" + std::to_wstring(i) + 
                              L" - 用于测试日志轮转功能的长消息内容，包含更多文本以快速达到轮转阈值。";
        logger.WriteLogContent(LogLevel::Info, message);
        
        // 每100条日志休眠一下，让轮转系统有时间处理
        if (i % 100 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::wcout << L"已写入 " << (i + 1) << L" 条日志" << std::endl;
        }
    }
    
    // 检查轮转任务状态
    std::wcout << L"⏳ 检查轮转任务状态..." << std::endl;
    std::wcout << L"待处理任务: " << logger.GetPendingRotationTasks() << std::endl;
    
    // 手动触发轮转
    std::wcout << L"🔄 手动触发轮转..." << std::endl;
    logger.ForceLogRotation();
    
    // 异步轮转示例
    std::wcout << L"🚀 异步轮转示例..." << std::endl;
    auto asyncResult = logger.ForceLogRotationAsync();
    
    // 等待异步轮转完成
    if (asyncResult.wait_for(std::chrono::seconds(5)) == std::future_status::ready) {
        bool success = asyncResult.get();
        std::wcout << L"异步轮转" << (success ? L"成功" : L"失败") << std::endl;
    } else {
        std::wcout << L"异步轮转超时" << std::endl;
    }
    
    // 获取当前配置
    auto currentConfig = logger.GetLogRotationConfig();
    std::wcout << L"📊 当前轮转配置:" << std::endl;
    std::wcout << L"   - 最大文件大小: " << currentConfig.maxFileSizeMB << L" MB" << std::endl;
    std::wcout << L"   - 最大归档文件: " << currentConfig.maxArchiveFiles << std::endl;
    std::wcout << L"   - 异步处理: " << (currentConfig.enableAsync ? L"是" : L"否") << std::endl;
    
    std::wcout << L"✨ 轮转系统演示完成!" << std::endl;
}

int main() {
    try {
        DemonstrateRotationSystem();
    }
    catch (const std::exception& e) {
        std::cerr << "演示过程中发生异常: " << e.what() << std::endl;
        return 1;
    }
    
    std::wcout << L"按任意键退出..." << std::endl;
    std::cin.get();
    return 0;
}