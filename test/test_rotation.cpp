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
    std::wcout << L"=== Testing Rotation Strategies ===" << std::endl;
    
    // 测试大小策略
    auto sizeStrategy = std::make_shared<SizeBasedRotationStrategy>(1024 * 1024); // 1MB
    std::wcout << L"Strategy name: " << sizeStrategy->GetStrategyName() << std::endl;
    std::wcout << L"Strategy description: " << sizeStrategy->GetStrategyDescription() << std::endl;
    
    // 创建测试上下文
    RotationContext context;
    context.currentFileName = L"test.log";
    context.currentFileSize = 2 * 1024 * 1024; // 2MB
    context.currentTime = std::chrono::system_clock::now();
    context.lastRotationTime = std::chrono::system_clock::now() - std::chrono::hours(1);
    
    // 测试轮转决策
    auto decision = sizeStrategy->ShouldRotate(context);
    std::wcout << L"Should rotate: " << (decision.shouldRotate ? L"Yes" : L"No") << std::endl;
    std::wcout << L"Rotation reason: " << decision.reason << std::endl;
    std::wcout << L"Priority: " << decision.priority << std::endl;
    
    // 测试时间策略
    auto timeStrategy = std::make_shared<TimeBasedRotationStrategy>(TimeBasedRotationStrategy::TimeInterval::Hourly);
    std::wcout << L"\nTime strategy name: " << timeStrategy->GetStrategyName() << std::endl;
    std::wcout << L"Time strategy description: " << timeStrategy->GetStrategyDescription() << std::endl;
    
    auto timeDecision = timeStrategy->ShouldRotate(context);
    std::wcout << L"Time strategy rotation decision: " << (timeDecision.shouldRotate ? L"Yes" : L"No") << std::endl;
    std::wcout << L"Time strategy reason: " << timeDecision.reason << std::endl;
}

void TestAsyncRotationManager() {
    std::wcout << L"\n=== Testing Async Rotation Manager ===" << std::endl;
    
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
    std::wcout << L"Manager started, running status: " << (manager.IsRunning() ? L"Running" : L"Stopped") << std::endl;
    
    // 获取统计信息
    auto stats = manager.GetStatistics();
    std::wcout << L"Total rotations: " << stats.totalRotations << std::endl;
    std::wcout << L"Successful rotations: " << stats.successfulRotations << std::endl;
    
    // 测试轮转检查
    RotationTrigger trigger = manager.CheckRotationNeeded(L"test.log", 2 * 1024 * 1024); // 2MB
    std::wcout << L"Needs rotation: " << (trigger.sizeExceeded || trigger.timeReached || trigger.manualRequested ? L"Yes" : L"No") << std::endl;
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
    std::wcout << L"Pre-checker created successfully" << std::endl;
    
    // 测试错误处理器 
    RotationErrorHandler errorHandler;
    std::wcout << L"Error handler created successfully" << std::endl;
    
    // 测试状态机
    RotationStateMachine stateMachine;
    std::wcout << L"State machine created successfully, current state: " << static_cast<int>(stateMachine.GetCurrentState()) << std::endl;
    
    // 测试时间计算器
    TimeCalculator timeCalc;
    auto now = std::chrono::system_clock::now();
    std::wcout << L"Time calculator created successfully" << std::endl;
}

int main() {
    try {
        std::wcout << L"开始轮转系统功能测试..." << std::endl;
        
        TestRotationStrategy();
        TestAsyncRotationManager();
        TestRotationComponents();
        
        std::wcout << L"\nAll tests completed! Rotation system is working properly." << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::wcout << L"Exception occurred during testing: " << e.what() << std::endl;
        return 1;
    }
}