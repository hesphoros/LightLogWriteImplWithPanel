/**
 * @file multioutput_json_config_demo.cpp
 * @brief 演示MultiOutputLogConfig的JSON序列化和反序列化功能
 * @details 展示如何保存和加载多输出日志配置到JSON文件
 */

#include <iostream>
#include <memory>
#include "log/LightLogWriteImpl.h"
#include "log/MultiOutputLogConfig.h"
#include "log/ConsoleLogOutput.h"
#include "log/FileLogOutput.h"
#include "log/BasicLogFormatter.h"

void DemonstrateJsonConfigSerialization() {
    std::wcout << L"=== Multi-Output JSON Configuration Demo ===" << std::endl;
    
    // 创建日志系统
    auto logger = std::make_shared<LightLogWrite_Impl>(1000);
    logger->SetMultiOutputEnabled(true);
    
    // 1. 创建和配置多个输出
    std::wcout << L"\n1. Creating and configuring outputs..." << std::endl;
    
    // 控制台输出
    auto consoleOutput = std::make_shared<ConsoleLogOutput>(L"Console", true, true, false);
    consoleOutput->Initialize();
    logger->AddLogOutput(consoleOutput);
    
    // 文件输出
    auto fileOutput = std::make_shared<FileLogOutput>(L"JsonDemo");
    fileOutput->Initialize(L"logs/json_config_demo.log");
    logger->AddLogOutput(fileOutput);
    
    std::wcout << L"✓ Added Console and File outputs" << std::endl;
    
    // 2. 保存配置到JSON文件
    std::wcout << L"\n2. Saving configuration to JSON file..." << std::endl;
    
    std::wstring configPath = L"config/multioutput_config.json";
    
    if (logger->SaveMultiOutputConfigToJson(configPath)) {
        std::wcout << L"✓ Configuration saved to: " << configPath << std::endl;
    } else {
        std::wcout << L"✗ Failed to save configuration" << std::endl;
        return;
    }
    
    // 3. 清除当前配置
    std::wcout << L"\n3. Clearing current configuration..." << std::endl;
    logger->SetMultiOutputEnabled(false);
    
    // 4. 从JSON文件加载配置
    std::wcout << L"\n4. Loading configuration from JSON file..." << std::endl;
    
    if (logger->LoadMultiOutputConfigFromJson(configPath)) {
        std::wcout << L"✓ Configuration loaded successfully" << std::endl;
        logger->SetMultiOutputEnabled(true);
    } else {
        std::wcout << L"✗ Failed to load configuration" << std::endl;
        return;
    }
    
    // 5. 测试配置是否正确加载
    std::wcout << L"\n5. Testing loaded configuration..." << std::endl;
    
    // 写入测试日志
    logger->WriteLogInfo(L"JSON配置加载测试 - 这条消息应该同时出现在控制台和文件中");
    logger->WriteLogWarning(L"JSON配置测试警告消息");
    logger->WriteLogError(L"JSON配置测试错误消息");
    
    std::wcout << L"✓ Test logs written with loaded configuration" << std::endl;
    
    // 6. 展示JSON文件内容
    std::wcout << L"\n6. JSON configuration file content:" << std::endl;
    std::wcout << L"   File: " << configPath << std::endl;
    std::wcout << L"   (Check the file to see the complete JSON structure)" << std::endl;
    
    std::wcout << L"\n=== Demo completed successfully ===" << std::endl;
}

void DemonstrateAdvancedJsonConfig() {
    std::wcout << L"\n=== Advanced JSON Configuration Demo ===" << std::endl;
    
    // 创建高级配置
    MultiOutputLogConfig advancedConfig;
    
    // 基础设置
    advancedConfig.enabled = true;
    advancedConfig.configVersion = L"2.0";
    advancedConfig.globalMinLevel = LogLevel::Debug;
    
    // 管理器配置
    advancedConfig.managerConfig.writeMode = OutputWriteMode::Parallel;
    advancedConfig.managerConfig.asyncQueueSize = 2000;
    advancedConfig.managerConfig.workerThreadCount = 4;
    advancedConfig.managerConfig.failFastOnError = false;
    advancedConfig.managerConfig.writeTimeout = 10.0;
    
    // 输出配置1：控制台输出
    OutputConfig consoleConfig;
    consoleConfig.name = L"AdvancedConsole";
    consoleConfig.type = L"Console";
    consoleConfig.enabled = true;
    consoleConfig.minLevel = LogLevel::Info;
    consoleConfig.config = L"useStderr=true;enableColors=true";
    
    // 格式化器配置
    consoleConfig.useFormatter = true;
    consoleConfig.formatterConfig.pattern = L"[{timestamp}] [{level}] {message}";
    consoleConfig.formatterConfig.timestampFormat = L"%Y-%m-%d %H:%M:%S";
    consoleConfig.formatterConfig.enableColors = true;
    consoleConfig.formatterConfig.enableThreadId = true;
    consoleConfig.formatterConfig.enableProcessId = false;
    consoleConfig.formatterConfig.enableSourceInfo = false;
    
    // 设置颜色映射
    consoleConfig.formatterConfig.levelColors[LogLevel::Info] = LogColor::Green;
    consoleConfig.formatterConfig.levelColors[LogLevel::Warning] = LogColor::Yellow;
    consoleConfig.formatterConfig.levelColors[LogLevel::Error] = LogColor::Red;
    consoleConfig.formatterConfig.levelColors[LogLevel::Critical] = LogColor::BrightRed;
    
    advancedConfig.outputs.push_back(consoleConfig);
    
    // 输出配置2：文件输出
    OutputConfig fileConfig;
    fileConfig.name = L"AdvancedFile";
    fileConfig.type = L"File";
    fileConfig.enabled = true;
    fileConfig.minLevel = LogLevel::Trace;
    fileConfig.config = L"filePath=logs/advanced_demo.log;maxSize=100MB;rotation=daily";
    
    // 文件格式化器配置
    fileConfig.useFormatter = true;
    fileConfig.formatterConfig.pattern = L"[{timestamp}] [{level}] [Thread:{threadId}] {message}";
    fileConfig.formatterConfig.timestampFormat = L"%Y-%m-%d %H:%M:%S.%f";
    fileConfig.formatterConfig.enableColors = false;  // 文件输出不需要颜色
    fileConfig.formatterConfig.enableThreadId = true;
    fileConfig.formatterConfig.enableProcessId = true;
    fileConfig.formatterConfig.enableSourceInfo = true;
    
    advancedConfig.outputs.push_back(fileConfig);
    
    // 保存高级配置
    std::wstring advancedConfigPath = L"config/advanced_multioutput_config.json";
    
    std::wcout << L"\n1. Saving advanced configuration..." << std::endl;
    if (MultiOutputConfigSerializer::SaveToFile(advancedConfig, advancedConfigPath)) {
        std::wcout << L"✓ Advanced configuration saved to: " << advancedConfigPath << std::endl;
    } else {
        std::wcout << L"✗ Failed to save advanced configuration" << std::endl;
        return;
    }
    
    // 加载并解析高级配置
    std::wcout << L"\n2. Loading and parsing advanced configuration..." << std::endl;
    MultiOutputLogConfig loadedConfig;
    
    if (MultiOutputConfigSerializer::LoadFromFile(advancedConfigPath, loadedConfig)) {
        std::wcout << L"✓ Advanced configuration loaded successfully" << std::endl;
        
        // 显示配置信息
        std::wcout << L"\nLoaded Configuration Details:" << std::endl;
        std::wcout << L"  - Version: " << loadedConfig.configVersion << std::endl;
        std::wcout << L"  - Enabled: " << (loadedConfig.enabled ? L"Yes" : L"No") << std::endl;
        std::wcout << L"  - Global Min Level: " << static_cast<int>(loadedConfig.globalMinLevel) << std::endl;
        std::wcout << L"  - Write Mode: " << static_cast<int>(loadedConfig.managerConfig.writeMode) << std::endl;
        std::wcout << L"  - Worker Threads: " << loadedConfig.managerConfig.workerThreadCount << std::endl;
        std::wcout << L"  - Output Count: " << loadedConfig.outputs.size() << std::endl;
        
        for (size_t i = 0; i < loadedConfig.outputs.size(); ++i) {
            const auto& output = loadedConfig.outputs[i];
            std::wcout << L"    Output " << (i + 1) << L": " << output.name 
                       << L" (" << output.type << L")" << std::endl;
            std::wcout << L"      - Enabled: " << (output.enabled ? L"Yes" : L"No") << std::endl;
            std::wcout << L"      - Min Level: " << static_cast<int>(output.minLevel) << std::endl;
            std::wcout << L"      - Use Formatter: " << (output.useFormatter ? L"Yes" : L"No") << std::endl;
            std::wcout << L"      - Pattern: " << output.formatterConfig.pattern << std::endl;
        }
    } else {
        std::wcout << L"✗ Failed to load advanced configuration" << std::endl;
    }
    
    std::wcout << L"\n=== Advanced Demo completed ===" << std::endl;
}

int main() {
    try {
        // 确保目录存在
        std::filesystem::create_directories("config");
        std::filesystem::create_directories("logs");
        
        // 运行基础演示
        DemonstrateJsonConfigSerialization();
        
        // 等待用户输入
        std::wcout << L"\nPress Enter to continue with advanced demo...";
        std::wcin.get();
        
        // 运行高级演示
        DemonstrateAdvancedJsonConfig();
        
        std::wcout << L"\n=== All demonstrations completed successfully ===" << std::endl;
        std::wcout << L"\nGenerated files:" << std::endl;
        std::wcout << L"  - config/multioutput_config.json" << std::endl;
        std::wcout << L"  - config/advanced_multioutput_config.json" << std::endl;
        std::wcout << L"  - logs/json_config_demo.log" << std::endl;
        std::wcout << L"  - logs/advanced_demo.log" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        return 1;
    }
    
    return 0;
}