#include "../include/log/LogFilterFactory.h"
#include "../include/log/MultiOutputLogConfig.h"
#include "../include/log/LightLogWriteImpl.h"
#include <iostream>
#include <memory>

/**
 * @brief 过滤器序列化功能演示
 * @details 展示如何创建、配置、序列化和反序列化各种类型的过滤器
 */

void DemonstrateFilterCreation() {
    std::wcout << L"\n=== 过滤器创建和基本配置演示 ===\n";
    
    // 初始化过滤器工厂
    LogFilterFactory::Initialize();
    
    // 创建级别过滤器
    auto levelFilter = LogFilterFactory::CreateFilter(L"Level");
    if (levelFilter) {
        levelFilter->SetEnabled(true);
        levelFilter->SetPriority(10);
        std::wcout << L"创建级别过滤器成功: " << levelFilter->GetFilterName() << L"\n";
        std::wcout << L"描述: " << levelFilter->GetDescription() << L"\n";
    }
    
    // 创建关键词过滤器
    auto keywordFilter = LogFilterFactory::CreateFilter(L"Keyword");
    if (keywordFilter) { 
        keywordFilter->SetEnabled(true);
        keywordFilter->SetPriority(5);
        std::wcout << L"创建关键词过滤器成功: " << keywordFilter->GetFilterName() << L"\n";
    }
    
    // 创建正则表达式过滤器
    auto regexFilter = LogFilterFactory::CreateFilter(L"Regex");
    if (regexFilter) {
        regexFilter->SetEnabled(true); 
        regexFilter->SetPriority(0);
        std::wcout << L"创建正则过滤器成功: " << regexFilter->GetFilterName() << L"\n";
    }
    
    // 显示所有注册的过滤器类型
    std::wcout << L"\n注册的过滤器类型:\n";
    auto types = LogFilterFactory::GetRegisteredTypes();
    for (const auto& type : types) {
        const auto* info = LogFilterFactory::GetTypeInfo(type);
        if (info) {
            std::wcout << L"- " << type << L": " << info->description << L"\n";
        }
    }
}

void DemonstrateFilterSerialization() {
    std::wcout << L"\n=== 过滤器序列化演示 ===\n";
    
    // 创建并配置级别过滤器
    auto levelFilterPtr = LogFilterFactory::CreateFilter(L"Level");
    auto levelFilter = dynamic_cast<LevelFilter*>(levelFilterPtr.get());
    if (levelFilter) {
        levelFilter->SetEnabled(true);
        levelFilter->SetPriority(10);
        levelFilter->SetMinLevel(LogLevel::Info);
        levelFilter->SetMaxLevel(LogLevel::Error);
        
        // 序列化过滤器
        nlohmann::json serialized = LogFilterFactory::SerializeFilter(levelFilter);
        std::wcout << L"级别过滤器序列化结果:\n";
        std::wcout << L"JSON: " << serialized.dump(2).c_str() << L"\n\n";
        
        // 反序列化过滤器
        auto deserializedFilter = LogFilterFactory::DeserializeFilter(serialized);
        if (deserializedFilter) {
            std::wcout << L"反序列化成功: " << deserializedFilter->GetFilterName() << L"\n";
            std::wcout << L"启用状态: " << (deserializedFilter->IsEnabled() ? L"是" : L"否") << L"\n";
            std::wcout << L"优先级: " << deserializedFilter->GetPriority() << L"\n";
        }
    }
    
    // 创建并配置关键词过滤器
    auto keywordFilterPtr = LogFilterFactory::CreateFilter(L"Keyword");
    auto keywordFilter = dynamic_cast<KeywordFilter*>(keywordFilterPtr.get());
    if (keywordFilter) {
        keywordFilter->SetEnabled(true);
        keywordFilter->SetPriority(5);
        keywordFilter->SetCaseSensitive(false);
        keywordFilter->AddIncludeKeyword(L"error");
        keywordFilter->AddIncludeKeyword(L"warning");
        keywordFilter->AddExcludeKeyword(L"debug");
        
        // 序列化关键词过滤器
        nlohmann::json serialized = LogFilterFactory::SerializeFilter(keywordFilter);
        std::wcout << L"关键词过滤器序列化结果:\n";
        std::wcout << L"JSON: " << serialized.dump(2).c_str() << L"\n\n";
    }
}

void DemonstrateMultiOutputFilterConfig() {
    std::wcout << L"\n=== 多输出过滤器配置演示 ===\n";
    
    // 创建多输出配置
    MultiOutputLogConfig config;
    config.enabled = true;
    config.globalMinLevel = LogLevel::Info;
    config.configVersion = L"1.0";
    
    // 创建文件输出配置（带级别过滤器）
    OutputConfig fileOutput;
    fileOutput.name = L"FileOutput";
    fileOutput.type = L"File";
    fileOutput.enabled = true;
    fileOutput.minLevel = LogLevel::Debug;
    fileOutput.config = L"{\"filePath\":\"logs/app.log\",\"maxFileSize\":10485760}";
    fileOutput.useFilter = true;
    fileOutput.filterType = L"Level";
    fileOutput.filterConfig = L"{\"minLevel\":\"Debug\",\"maxLevel\":\"Fatal\",\"hasMaxLevel\":true}";
    config.outputs.push_back(fileOutput);
    
    // 创建控制台输出配置（带关键词过滤器）
    OutputConfig consoleOutput;
    consoleOutput.name = L"ConsoleOutput";
    consoleOutput.type = L"Console";
    consoleOutput.enabled = true;
    consoleOutput.minLevel = LogLevel::Info;
    consoleOutput.config = L"{\"useColors\":true}";
    consoleOutput.useFilter = true;
    consoleOutput.filterType = L"Keyword";
    consoleOutput.filterConfig = L"{\"caseSensitive\":false,\"includeKeywords\":[\"error\",\"warning\"]}";
    config.outputs.push_back(consoleOutput);
    
    // 序列化配置
    nlohmann::json configJson = MultiOutputConfigSerializer::ToJson(config);
    std::wcout << L"多输出配置序列化结果:\n";
    std::wcout << L"JSON: " << configJson.dump(2).c_str() << L"\n\n";
    
    // 反序列化配置
    MultiOutputLogConfig deserializedConfig = MultiOutputConfigSerializer::FromJson(configJson);
    std::wcout << L"反序列化结果:\n";
    std::wcout << L"配置版本: " << deserializedConfig.configVersion.c_str() << L"\n";
    std::wcout << L"启用状态: " << (deserializedConfig.enabled ? L"是" : L"否") << L"\n";
    std::wcout << L"输出数量: " << deserializedConfig.outputs.size() << L"\n";
    
    for (const auto& output : deserializedConfig.outputs) {
        std::wcout << L"- 输出: " << output.name.c_str() << L", 类型: " << output.type.c_str();
        if (output.useFilter) {
            std::wcout << L", 过滤器: " << output.filterType.c_str();
        }
        std::wcout << L"\n";
    }
}

void DemonstrateConfigFileOperations() {
    std::wcout << L"\n=== 配置文件操作演示 ===\n";
    
    // 创建示例配置
    MultiOutputLogConfig config;
    config.enabled = true;
    config.globalMinLevel = LogLevel::Debug;
    config.configVersion = L"1.0";
    
    // 添加带过滤器的输出配置
    OutputConfig output;
    output.name = L"FilteredFileOutput";
    output.type = L"File";
    output.enabled = true;
    output.minLevel = LogLevel::Info;
    output.config = L"{\"filePath\":\"logs/filtered.log\"}";
    output.useFilter = true;
    output.filterType = L"Level";
    output.filterConfig = L"{\"minLevel\":\"Info\",\"maxLevel\":\"Error\"}";
    config.outputs.push_back(output);
    
    // 保存到文件
    std::wstring configFile = L"config/demo_filter_config.json";
    bool saveResult = MultiOutputConfigSerializer::SaveToFile(config, configFile);
    std::wcout << L"保存配置到文件 " << configFile.c_str() << L": " 
               << (saveResult ? L"成功" : L"失败") << L"\n";
    
    // 从文件加载
    MultiOutputLogConfig loadedConfig;
    bool loadResult = MultiOutputConfigSerializer::LoadFromFile(configFile, loadedConfig);
    std::wcout << L"从文件加载配置: " << (loadResult ? L"成功" : L"失败") << L"\n";
    
    if (loadResult) {
        std::wcout << L"加载的配置信息:\n";
        std::wcout << L"- 版本: " << loadedConfig.configVersion.c_str() << L"\n";
        std::wcout << L"- 输出数量: " << loadedConfig.outputs.size() << L"\n";
        for (const auto& out : loadedConfig.outputs) {
            std::wcout << L"  * " << out.name.c_str() << L" (过滤器: " 
                       << (out.useFilter ? out.filterType.c_str() : L"无") << L")\n";
        }
    }
}

int main() {
    try {
        std::wcout << L"=== 日志过滤器序列化功能演示 ===\n";
        
        DemonstrateFilterCreation();
        DemonstrateFilterSerialization();
        DemonstrateMultiOutputFilterConfig();
        DemonstrateConfigFileOperations();
        
        std::wcout << L"\n=== 演示完成 ===\n";
        std::wcout << L"过滤器序列化功能已成功实现，包括:\n";
        std::wcout << L"1. 过滤器工厂模式和注册机制\n";
        std::wcout << L"2. 完整的序列化/反序列化支持\n";
        std::wcout << L"3. 多输出配置中的过滤器集成\n";
        std::wcout << L"4. 配置文件的保存和加载\n";
        std::wcout << L"5. 支持Level、Keyword、Regex、RateLimit、Thread等过滤器类型\n";
        
    } catch (const std::exception& e) {
        std::wcerr << L"演示过程中发生异常: " << e.what() << L"\n";
        return 1;
    }
    
    return 0;
}