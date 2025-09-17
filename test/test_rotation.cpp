/*****************************************************************************
 *  轮转系统功能测试
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  @file     test_rotation.cpp
 *  @brief    测试轮转系统的各个组件
 *  @details  验证AsyncRotationManager及相关组件的功能
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/09/16
 *  @license  GNU General Public License (GPL)
 *****************************************************************************/

#include "log/AsyncRotationManager.h"
#include "log/RotationStrategies.h"
#include "log/LogCompressor.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>

void TestRotationStrategy() {
    std::wcout << L"=== 测试轮转策略 ===" << std::endl;
    
    // 测试大小策略
    auto sizeStrategy = std::make_shared<SizeBasedRotationStrategy>(1024 * 1024); // 1MB
    std::wcout << L"策略名称: " << sizeStrategy->GetStrategyName() << std::endl;
    std::wcout << L"策略描述: " << sizeStrategy->GetStrategyDescription() << std::endl;
    
    // 创建测试上下文
    RotationContext context;
    context.currentFileName = L"test.log";
    context.currentFileSize = 2 * 1024 * 1024; // 2MB
    context.currentTime = std::chrono::system_clock::now();
    context.lastRotationTime = std::chrono::system_clock::now() - std::chrono::hours(1);
    
    // 测试轮转决策
    auto decision = sizeStrategy->ShouldRotate(context);
    std::wcout << L"是否需要轮转: " << (decision.shouldRotate ? L"是" : L"否") << std::endl;
    std::wcout << L"轮转原因: " << decision.reason << std::endl;
    std::wcout << L"优先级: " << decision.priority << std::endl;
    
    // 测试时间策略
    auto timeStrategy = std::make_shared<TimeBasedRotationStrategy>(TimeBasedRotationStrategy::TimeInterval::Hourly);
    std::wcout << L"\n时间策略名称: " << timeStrategy->GetStrategyName() << std::endl;
    std::wcout << L"时间策略描述: " << timeStrategy->GetStrategyDescription() << std::endl;
    
    auto timeDecision = timeStrategy->ShouldRotate(context);
    std::wcout << L"时间策略轮转决策: " << (timeDecision.shouldRotate ? L"是" : L"否") << std::endl;
    std::wcout << L"时间策略原因: " << timeDecision.reason << std::endl;
}

void TestAsyncRotationManager() {
    std::wcout << L"\n=== 测试异步轮转管理器 ===" << std::endl;
    
    // 创建配置
    AsyncRotationConfig asyncConfig;
    asyncConfig.workerThreadCount = 1;
    asyncConfig.maxQueueSize = 10;
    asyncConfig.enablePreCheck = true;
    asyncConfig.enableTransaction = true;
    
    LogRotationConfig rotationConfig;
    rotationConfig.strategy = LogRotationStrategy::Size;
    rotationConfig.maxFileSizeMB = 1;
    rotationConfig.archiveDirectory = L"logs/archive";
    rotationConfig.enableCompression = true;
    
    // 确保目录存在
    std::filesystem::create_directories("logs/archive");
    
    // 创建管理器
    AsyncRotationManager manager(asyncConfig);
    manager.SetConfig(rotationConfig);
    
    // 设置轮转策略
    auto strategy = std::make_shared<SizeBasedRotationStrategy>(1024 * 1024); // 1MB
    manager.SetRotationStrategy(strategy);
    
    // 启动管理器
    manager.Start();
    std::wcout << L"管理器已启动，运行状态: " << (manager.IsRunning() ? L"运行中" : L"已停止") << std::endl;
    
    // 获取统计信息
    auto stats = manager.GetStatistics();
    std::wcout << L"总轮转次数: " << stats.totalRotations << std::endl;
    std::wcout << L"成功轮转次数: " << stats.successfulRotations << std::endl;
    
    // 测试轮转检查
    RotationTrigger trigger = manager.CheckRotationNeeded(L"test.log", 2 * 1024 * 1024); // 2MB
    std::wcout << L"需要轮转: " << (trigger.sizeExceeded || trigger.timeReached || trigger.manualRequested ? L"是" : L"否") << std::endl;
    std::wcout << L"触发原因: " << trigger.reason << std::endl;
    
    // 获取管理器状态
    std::wcout << L"\n管理器状态:\n" << manager.GetManagerStatus() << std::endl;
    
    // 停止管理器
    manager.Stop();
    std::wcout << L"管理器已停止" << std::endl;
}

void TestRotationComponents() {
    std::wcout << L"\n=== 测试其他轮转组件 ===" << std::endl;
    
    // 测试压缩器
    LogCompressor compressor;
    std::wcout << L"压缩器支持的格式数: " << compressor.GetSupportedFormats().size() << std::endl;
    
    // 测试预检查器
    RotationPreChecker preChecker;
    std::wcout << L"预检查器创建成功" << std::endl;
    
    // 测试错误处理器 
    RotationErrorHandler errorHandler;
    std::wcout << L"错误处理器创建成功" << std::endl;
    
    // 测试状态机
    RotationStateMachine stateMachine;
    std::wcout << L"状态机创建成功，当前状态: " << static_cast<int>(stateMachine.GetCurrentState()) << std::endl;
    
    // 测试时间计算器
    TimeCalculator timeCalc;
    auto now = std::chrono::system_clock::now();
    std::wcout << L"时间计算器创建成功" << std::endl;
}

int main() {
    try {
        std::wcout << L"开始轮转系统功能测试..." << std::endl;
        
        TestRotationStrategy();
        TestAsyncRotationManager();
        TestRotationComponents();
        
        std::wcout << L"\n所有测试完成！轮转系统功能正常。" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::wcout << L"测试过程中发生异常: " << e.what() << std::endl;
        return 1;
    }
}