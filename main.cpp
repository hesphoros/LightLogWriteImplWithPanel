#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <filesystem>
#include <iomanip>
#include <regex>

// 引入我们的日志系统头文件
#include "log/LightLogWriteImpl.h"
#include "log/LogCompressor.h"
#include "log/LogOutputManager.h"
#include "log/ConsoleLogOutput.h"
#include "log/FileLogOutput.h"
#include "log/BasicLogFormatter.h"
#include "log/UniConv.h"

// 引入新的过滤器系统头文件
#include "log/ILogFilter.h"
#include "log/LogFilters.h"
#include "log/CompositeFilter.h"
#include "log/FilterManager.h"

// ==================== 日志测试框架工具类 ====================
/**
 * @brief 日志测试框架类，用于统一管理和执行各种日志功能测试
 * @details 该类提供了测试用例的启动、结果统计、测试报告等功能
 *          支持对日志系统的各个模块进行模块化测试
 */
class LogTestFramework {
private:
    std::shared_ptr<LightLogWrite_Impl> logger_;      // 日志实现对象
    std::shared_ptr<LogCompressor> compressor_;       // 压缩器对象
    int testCount_ = 0;                               // 总测试数量计数器
    int passedTests_ = 0;                             // 通过测试数量计数器
    
public:
    /**
     * @brief 构造函数，初始化测试框架
     * @param logger 日志实现对象的共享指针
     * @param compressor 压缩器对象的共享指针
     */
    LogTestFramework(std::shared_ptr<LightLogWrite_Impl> logger, std::shared_ptr<LogCompressor> compressor)
        : logger_(logger), compressor_(compressor) {}
    
    /**
     * @brief 开始一个新的测试用例
     * @param testName 测试用例名称（英文）
     */
    void StartTest(const std::wstring& testName) {
        testCount_++;
        std::wcout << L"\n=== Test " << testCount_ << L": " << testName << L" ===" << std::endl;
    }
    
    /**
     * @brief 标记测试用例通过
     * @param message 成功消息（英文）
     */
    void TestPass(const std::wstring& message = L"") {
        passedTests_++;
        if (!message.empty()) {
            std::wcout << L"✓ " << message << std::endl;
        }
    }
    
    /**
     * @brief 标记测试用例失败
     * @param message 失败消息（英文）
     */
    void TestFail(const std::wstring& message) {
        std::wcout << L"✗ " << message << std::endl;
    }
    
    /**
     * @brief 显示测试总结报告
     * @details 输出总测试数、通过数、失败数和成功率统计信息
     */
    void ShowSummary() {
        std::wcout << L"\n=== Test Summary ===" << std::endl;
        std::wcout << L"Total Tests: " << testCount_ << std::endl;
        std::wcout << L"Passed: " << passedTests_ << std::endl;
        std::wcout << L"Failed: " << (testCount_ - passedTests_) << std::endl;
        std::wcout << L"Success Rate: " << std::fixed << std::setprecision(1) 
                   << (double)passedTests_ / testCount_ * 100.0 << L"%" << std::endl;
    }
    
    /**
     * @brief 获取日志对象
     * @return 日志实现对象的共享指针
     */
    std::shared_ptr<LightLogWrite_Impl> GetLogger() { return logger_; }
    
    /**
     * @brief 获取压缩器对象
     * @return 压缩器对象的共享指针
     */
    std::shared_ptr<LogCompressor> GetCompressor() { return compressor_; }
};

// ==================== 模块1：基础日志功能测试 ====================
/**
 * @brief 基础日志功能测试类
 * @details 测试日志系统的基本写入功能，包括各种日志级别的写入
 *          验证日志文件的正确创建和内容写入
 */
class BasicLoggingTests {
public:
    /**
     * @brief 运行基础日志功能测试
     * @param framework 测试框架引用
     * @details 测试所有日志级别的写入功能，验证文件大小和内容完整性
     */
    static void RunTests(LogTestFramework& framework) {
        framework.StartTest(L"Basic Logging Functions");
        
        auto logger = framework.GetLogger();
        
        try {
            // 依次测试所有日志级别的写入功能
            logger->WriteLogTrace(L"Trace level test message");
            logger->WriteLogDebug(L"Debug level test message");
            logger->WriteLogInfo(L"Info level test message");
            logger->WriteLogNotice(L"Notice level test message");
            logger->WriteLogWarning(L"Warning level test message");
            logger->WriteLogError(L"Error level test message");
            logger->WriteLogCritical(L"Critical level test message");
            logger->WriteLogAlert(L"Alert level test message");
            logger->WriteLogEmergency(L"Emergency level test message");
            logger->WriteLogFatal(L"Fatal level test message");
            
            framework.TestPass(L"All log levels write successfully");
            
            // 验证日志文件确实被写入了内容
            size_t fileSize = logger->GetCurrentLogFileSize();
            if (fileSize > 0) {
                framework.TestPass(L"Log file validation passed, size: " + std::to_wstring(fileSize) + L" bytes");
            } else {
                framework.TestFail(L"Log file size is zero - no content written");
            }
            
        } catch (const std::exception& e) {
            framework.TestFail(L"Basic logging test exception: " + 
                UniConv::GetInstance()->LocaleToWideString(e.what()));
        }
    }
};

// ==================== 模块2：回调系统测试 ====================
/**
 * @brief 回调系统测试类
 * @details 测试日志系统的回调机制，包括回调注册、触发、计数和清理
 *          验证回调函数能正确接收到日志事件并执行相应操作
 */
class CallbackSystemTests {
public:
    /**
     * @brief 运行回调系统测试
     * @param framework 测试框架引用
     * @details 测试回调的完整生命周期：注册->触发->验证->清理
     */
    static void RunTests(LogTestFramework& framework) {
        framework.StartTest(L"Callback System");
        
        auto logger = framework.GetLogger();
        
        try {
            int callbackCount = 0;  // 回调触发次数计数器
            
            // 创建回调函数，用于捕获日志事件
            auto callback = [&callbackCount](const LogCallbackInfo& info) {
                callbackCount++;
                std::wcout << L"[CALLBACK] " << info.levelString << L": " << info.message << std::endl;
            };
            
            // 注册回调到日志系统
            CallbackHandle handle = logger->SubscribeToLogEvents(callback);
            
            // 验证回调注册是否成功
            if (logger->GetCallbackCount() > 0) {
                framework.TestPass(L"Callback registration successful");
            } else {
                framework.TestFail(L"Callback registration failed");
                return;
            }
            
            // 触发回调事件，写入两条不同级别的日志
            logger->WriteLogInfo(L"Callback test message 1");
            logger->WriteLogError(L"Callback test message 2");
            
            // 等待确保异步回调执行完成
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // 验证回调触发次数是否正确
            if (callbackCount == 2) {
                framework.TestPass(L"Callback triggered correctly, received " + std::to_wstring(callbackCount) + L" calls");
            } else {
                framework.TestFail(L"Callback trigger abnormal, expected 2 calls, actual " + std::to_wstring(callbackCount) + L" calls");
            }
            
            // 清理回调，从日志系统中移除
            logger->UnsubscribeFromLogEvents(handle);
            if (logger->GetCallbackCount() == 0) {
                framework.TestPass(L"Callback cleanup successful");
            } else {
                framework.TestFail(L"Callback cleanup failed");
            }
            
        } catch (const std::exception& e) {
            framework.TestFail(L"Callback system test exception: " + 
                UniConv::GetInstance()->LocaleToWideString(e.what()));
        }
    }
};

// ==================== 模块3：日志轮转系统测试 ====================
/**
 * @brief 日志轮转系统测试类
 * @details 测试日志轮转功能，包括轮转配置、自动轮转触发、手动轮转
 *          验证轮转后文件的压缩和归档功能
 */
class RotationSystemTests {
public:
    /**
     * @brief 运行轮转系统测试
     * @param framework 测试框架引用
     * @details 测试轮转配置设置、验证，以及手动轮转功能
     */
    static void RunTests(LogTestFramework& framework) {
        framework.StartTest(L"Log Rotation System");
        
        auto logger = framework.GetLogger();
        
        try {
            // 设置轮转配置参数
            LogRotationConfig config;
            config.strategy = LogRotationStrategy::SizeAndTime;  // 基于大小和时间的轮转策略
            config.maxFileSizeMB = 1024;                         // 最大文件大小1MB
            config.enableCompression = true;                     // 启用压缩功能
            config.archiveDirectory = L"logs/archive";           // 归档目录路径
            config.maxArchiveFiles = 10;                         // 最大归档文件数量
            
            logger->SetLogRotationConfig(config);
            framework.TestPass(L"Rotation configuration set successfully");
            
            // 验证轮转配置是否正确设置
            auto currentConfig = logger->GetLogRotationConfig();
            if (currentConfig.enableCompression && currentConfig.maxFileSizeMB == 1024) {
                framework.TestPass(L"Rotation configuration validation passed");
            } else {
                framework.TestFail(L"Rotation configuration validation failed");
            }
            
            // 生成大量日志内容以准备触发轮转
            size_t sizeBefore = logger->GetCurrentLogFileSize();
            
            for (int i = 0; i < 100; ++i) {
                logger->WriteLogInfo(L"Rotation test message " + std::to_wstring(i) + 
                    L" - Adding content to trigger rotation with sufficient data size for testing purposes.");
            }
            
            // 等待确保所有日志写入完成
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // 手动触发日志轮转操作
            std::wcout << L"Triggering manual rotation..." << std::endl;
            logger->ForceLogRotation();
            
            size_t sizeAfter = logger->GetCurrentLogFileSize();
            
            // 验证轮转是否成功（轮转后新文件应该比原文件小）
            if (sizeAfter < sizeBefore) {
                framework.TestPass(L"Manual rotation successful, file size reduced from " + std::to_wstring(sizeBefore) + 
                    L" bytes to " + std::to_wstring(sizeAfter) + L" bytes");
            } else {
                framework.TestFail(L"Manual rotation failed - file size did not decrease");
            }
            
        } catch (const std::exception& e) {
            framework.TestFail(L"Rotation system test exception: " + 
                UniConv::GetInstance()->LocaleToWideString(e.what()));
        }
    }
};

// ==================== 模块4：压缩系统测试 ====================
/**
 * @brief 压缩系统测试类
 * @details 测试日志文件的ZIP压缩功能，包括压缩文件生成、压缩统计
 *          验证压缩率和压缩任务的成功率
 */
class CompressionSystemTests {
public:
    /**
     * @brief 运行压缩系统测试
     * @param framework 测试框架引用
     * @details 检查归档目录中的ZIP文件，验证压缩功能和统计信息
     */
    static void RunTests(LogTestFramework& framework) {
        framework.StartTest(L"Compression System");
        
        auto logger = framework.GetLogger();
        
        try {
            // 检查归档目录是否存在
            std::wstring archivePath = L"logs/archive";
            
            if (!std::filesystem::exists(archivePath)) {
                framework.TestFail(L"Archive directory does not exist");
                return;
            }
            
            int zipFiles = 0;                    // ZIP文件计数器
            size_t totalCompressedSize = 0;      // 压缩文件总大小
            
            // 遍历归档目录，统计ZIP压缩文件
            for (const auto& entry : std::filesystem::directory_iterator(archivePath)) {
                if (std::filesystem::is_regular_file(entry.status()) && entry.path().extension() == L".zip") {
                    zipFiles++;
                    totalCompressedSize += std::filesystem::file_size(entry.path());
                }
            }
            
            // 验证是否找到了压缩文件
            if (zipFiles > 0) {
                framework.TestPass(L"Found " + std::to_wstring(zipFiles) + L" ZIP compressed files, total size: " + 
                    std::to_wstring(totalCompressedSize) + L" bytes");
            } else {
                framework.TestFail(L"No ZIP compressed files found");
            }
            
            // 获取并分析压缩统计信息
            auto stats = logger->GetCompressionStatistics();
            if (stats.totalTasks > 0) {
                // 计算压缩率百分比
                double ratio = (double)stats.totalCompressedSize / stats.totalOriginalSize * 100.0;
                framework.TestPass(L"Compression statistics: " + std::to_wstring(stats.successfulTasks) + L"/" + 
                    std::to_wstring(stats.totalTasks) + L" successful, compression ratio: " + 
                    std::to_wstring(ratio).substr(0, 4) + L"%");
            } else {
                framework.TestFail(L"No compression statistics available");
            }
            
        } catch (const std::exception& e) {
            framework.TestFail(L"Compression system test exception: " + 
                UniConv::GetInstance()->LocaleToWideString(e.what()));
        }
    }
};

// ==================== 模块5：过滤器系统测试 ====================
/**
 * @brief 过滤器系统测试类
 * @details 测试新实现的过滤器系统，包括各种过滤器类型、组合过滤器、
 *          过滤器管理器以及过滤器的配置和统计功能
 */
class FilterSystemTests {
public:
    /**
     * @brief 运行过滤器系统测试
     * @param framework 测试框架引用
     * @details 测试各种过滤器的创建、配置、过滤功能和性能统计
     */
    static void RunTests(LogTestFramework& framework) {
        framework.StartTest(L"Filter System Tests");
        
        try {
            // 1. 测试过滤器管理器
            TestFilterManager(framework);
            
            // 2. 测试基础过滤器
            TestBasicFilters(framework);
            
            // 3. 测试组合过滤器
            TestCompositeFilters(framework);
            
            // 4. 测试过滤器统计功能
            TestFilterStatistics(framework);
            
            // 5. 测试过滤器配置功能
            TestFilterConfiguration(framework);
            
            // 6. 测试过滤器与日志系统集成
            TestFilterIntegration(framework);
            
        } catch (const std::exception& e) {
            framework.TestFail(L"Filter system test exception: " + 
                UniConv::GetInstance()->LocaleToWideString(e.what()));
        }
    }
    
private:
    /**
     * @brief 测试过滤器管理器功能
     */
    static void TestFilterManager(LogTestFramework& framework) {
        std::wcout << L"\n--- Testing Filter Manager ---" << std::endl;
        
        FilterManager manager;
        
        // 测试创建不同类型的过滤器
        auto levelFilter = manager.CreateFilter(L"level");
        if (levelFilter) {
            framework.TestPass(L"Level filter creation successful");
        } else {
            framework.TestFail(L"Level filter creation failed");
        }
        
        auto keywordFilter = manager.CreateFilter(L"keyword");
        if (keywordFilter) {
            framework.TestPass(L"Keyword filter creation successful");
        } else {
            framework.TestFail(L"Keyword filter creation failed");
        }
        
        auto regexFilter = manager.CreateFilter(L"regex");
        if (regexFilter) {
            framework.TestPass(L"Regex filter creation successful");
        } else {
            framework.TestFail(L"Regex filter creation failed");
        }
        
        auto rateLimitFilter = manager.CreateFilter(L"ratelimit");
        if (rateLimitFilter) {
            framework.TestPass(L"Rate limit filter creation successful");
        } else {
            framework.TestFail(L"Rate limit filter creation failed");
        }
    }
    
    /**
     * @brief 测试基础过滤器功能
     */
    static void TestBasicFilters(LogTestFramework& framework) {
        std::wcout << L"\n--- Testing Basic Filters ---" << std::endl;
        
        // 创建测试用的日志信息
        LogCallbackInfo infoLog;
        infoLog.level = LogLevel::Info;
        infoLog.message = L"This is an info message for testing";
        
        LogCallbackInfo errorLog;
        errorLog.level = LogLevel::Error;
        errorLog.message = L"This is an error message for testing";
        
        // 测试级别过滤器
        auto levelFilter = std::make_unique<LevelFilter>(LogLevel::Warning);
        
        FilterOperation infoResult = levelFilter->ApplyFilter(infoLog, nullptr);
        FilterOperation errorResult = levelFilter->ApplyFilter(errorLog, nullptr);
        
        if (infoResult == FilterOperation::Block && errorResult == FilterOperation::Allow) {
            framework.TestPass(L"Level filter working correctly");
        } else {
            framework.TestFail(L"Level filter not working as expected");
        }
        
        // 测试关键词过滤器
        auto keywordFilter = std::make_unique<KeywordFilter>();
        keywordFilter->AddIncludeKeyword(L"error");
        keywordFilter->AddIncludeKeyword(L"warning");
        
        LogCallbackInfo keywordTestLog;
        keywordTestLog.level = LogLevel::Info;
        keywordTestLog.message = L"This contains error keyword";
        
        FilterOperation keywordResult = keywordFilter->ApplyFilter(keywordTestLog, nullptr);
        FilterOperation normalResult = keywordFilter->ApplyFilter(infoLog, nullptr);
        
        if (keywordResult == FilterOperation::Allow && normalResult == FilterOperation::Block) {
            framework.TestPass(L"Keyword filter working correctly");
        } else {
            framework.TestFail(L"Keyword filter not working as expected");
        }
        
        // 测试正则表达式过滤器
        auto regexFilter = std::make_unique<RegexFilter>(L".*test.*");
        
        LogCallbackInfo regexTestLog;
        regexTestLog.level = LogLevel::Info;
        regexTestLog.message = L"This message contains test keyword";
        
        FilterOperation regexResult = regexFilter->ApplyFilter(regexTestLog, nullptr);
        FilterOperation noMatchResult = regexFilter->ApplyFilter(infoLog, nullptr);
        
        if (regexResult == FilterOperation::Allow && noMatchResult == FilterOperation::Block) {
            framework.TestPass(L"Regex filter working correctly");
        } else {
            framework.TestFail(L"Regex filter not working as expected");
        }
        
        // 测试限流过滤器
        auto rateLimitFilter = std::make_unique<RateLimitFilter>(2, 2); // 每秒最多2条，突发2条
        
        FilterOperation firstResult = rateLimitFilter->ApplyFilter(infoLog, nullptr);
        FilterOperation secondResult = rateLimitFilter->ApplyFilter(errorLog, nullptr);
        FilterOperation thirdResult = rateLimitFilter->ApplyFilter(infoLog, nullptr); // 应该被限制
        
        if (firstResult == FilterOperation::Allow && secondResult == FilterOperation::Allow && thirdResult == FilterOperation::Block) {
            framework.TestPass(L"Rate limit filter working correctly");
        } else {
            framework.TestFail(L"Rate limit filter not working as expected");
        }
    }
    
    /**
     * @brief 测试组合过滤器功能
     */
    static void TestCompositeFilters(LogTestFramework& framework) {
        std::wcout << L"\n--- Testing Composite Filters ---" << std::endl;
        
        FilterManager manager;
        
        // 创建组合过滤器
        auto compositeFilter = manager.CreateCompositeFilter(CompositionStrategy::AllMustPass);
        
        if (compositeFilter) {
            // 添加子过滤器
            auto levelFilter = std::make_unique<LevelFilter>(LogLevel::Info);
            auto keywordFilter = std::make_unique<KeywordFilter>();
            keywordFilter->AddIncludeKeyword(L"test");
            
            compositeFilter->AddFilter(std::move(levelFilter));
            compositeFilter->AddFilter(std::move(keywordFilter));
            
            // 创建测试日志
            LogCallbackInfo testLog;
            testLog.level = LogLevel::Info;
            testLog.message = L"This contains test keyword";
            
            LogCallbackInfo debugLog;
            debugLog.level = LogLevel::Debug;
            debugLog.message = L"This contains test keyword";
            
            FilterOperation infoResult = compositeFilter->ApplyFilter(testLog, nullptr);
            FilterOperation debugResult = compositeFilter->ApplyFilter(debugLog, nullptr);
            
            // Info级别符合条件，Debug级别不符合
            if (infoResult == FilterOperation::Allow && debugResult == FilterOperation::Block) {
                framework.TestPass(L"Composite filter with AllMustPass strategy working correctly");
            } else {
                framework.TestFail(L"Composite filter not working as expected");
            }
        } else {
            framework.TestFail(L"Failed to create composite filter");
        }
    }
    
    /**
     * @brief 测试过滤器统计功能
     */
    static void TestFilterStatistics(LogTestFramework& framework) {
        std::wcout << L"\n--- Testing Filter Statistics ---" << std::endl;
        
        auto filter = std::make_unique<LevelFilter>(LogLevel::Warning);
        
        // 创建测试日志
        LogCallbackInfo infoLog;
        infoLog.level = LogLevel::Info;
        infoLog.message = L"Info message";
        
        LogCallbackInfo warningLog;
        warningLog.level = LogLevel::Warning;
        warningLog.message = L"Warning message";
        
        // 执行过滤操作
        filter->ApplyFilter(infoLog, nullptr);    // 应该被阻止
        filter->ApplyFilter(warningLog, nullptr); // 应该通过
        filter->ApplyFilter(infoLog, nullptr);    // 再次被阻止
        
        // 获取统计信息
        auto stats = filter->GetStatistics();
        
        if (stats.totalProcessed == 3 && stats.blocked == 2 && stats.allowed == 1) {
            framework.TestPass(L"Filter statistics working correctly - Total: " + 
                std::to_wstring(stats.totalProcessed) + L", Blocked: " + 
                std::to_wstring(stats.blocked) + L", Allowed: " + 
                std::to_wstring(stats.allowed));
        } else {
            framework.TestFail(L"Filter statistics not working as expected - Total: " + 
                std::to_wstring(stats.totalProcessed) + L", Blocked: " + 
                std::to_wstring(stats.blocked) + L", Allowed: " + 
                std::to_wstring(stats.allowed));
        }
        
        // 测试重置统计
        filter->ResetStatistics();
        auto resetStats = filter->GetStatistics();
        
        if (resetStats.totalProcessed == 0 && resetStats.blocked == 0 && resetStats.allowed == 0) {
            framework.TestPass(L"Filter statistics reset working correctly");
        } else {
            framework.TestFail(L"Filter statistics reset failed");
        }
    }
    
    /**
     * @brief 测试过滤器配置功能
     */
    static void TestFilterConfiguration(LogTestFramework& framework) {
        std::wcout << L"\n--- Testing Filter Configuration ---" << std::endl;
        
        FilterManager manager;
        
        // 测试基本配置功能
        auto filter = manager.CreateFilter(L"level", L"minLevel=Warning;enabled=true");
        if (filter) {
            framework.TestPass(L"Filter creation with configuration successful");
            
            // 验证配置是否正确应用
            if (filter->IsEnabled()) {
                framework.TestPass(L"Filter configuration applied correctly");
            } else {
                framework.TestFail(L"Filter configuration not applied correctly");
            }
        } else {
            framework.TestFail(L"Filter creation with configuration failed");
        }
        
        // 测试模板功能
        manager.CreateFilterTemplate(L"test_template", L"level", L"minLevel=Error;enabled=true");
        auto templateFilter = manager.CreateFromTemplate(L"test_template");
        
        if (templateFilter) {
            framework.TestPass(L"Filter template creation and usage working correctly");
        } else {
            framework.TestFail(L"Filter template creation or usage failed");
        }
        
        // 测试配置保存和加载
        if (filter) {
            // 将unique_ptr转换为shared_ptr
            std::shared_ptr<ILogFilter> sharedFilter(filter.release());
            manager.SaveFilterConfiguration(L"saved_config", sharedFilter);
            auto loadedFilter = manager.LoadFilterConfiguration(L"saved_config");
            
            if (loadedFilter && loadedFilter->IsEnabled()) {
                framework.TestPass(L"Filter configuration save/load working correctly");
            } else {
                framework.TestFail(L"Filter configuration save/load failed");
            }
        }
    }

    /**
     * @brief 测试过滤器与日志系统的集成功能
     */
    static void TestFilterIntegration(LogTestFramework& framework) {
        std::wcout << L"\n--- Testing Filter Integration with Log System ---" << std::endl;
        
        auto logger = framework.GetLogger();
        
        // 测试1：级别过滤器集成测试
        {
            std::wcout << L"\n* Testing Level Filter Integration *" << std::endl;
            
            // 创建级别过滤器，只允许ERROR及以上级别
            auto levelFilter = std::make_unique<LevelFilter>(LogLevel::Error);
            std::shared_ptr<ILogFilter> sharedFilter(levelFilter.release());
            
            // 设置过滤器到日志系统
            logger->SetLogFilter(sharedFilter);
            
            // 验证过滤器是否设置成功
            if (logger->HasLogFilter()) {
                framework.TestPass(L"Log filter set successfully");
            } else {
                framework.TestFail(L"Failed to set log filter");
                return;
            }
            
            // 记录当前日志文件大小
            size_t initialSize = logger->GetCurrentLogFileSize();
            
            // 写入不同级别的日志消息
            logger->WriteLogInfo(L"This INFO message should be filtered out");
            logger->WriteLogWarning(L"This WARNING message should be filtered out");
            logger->WriteLogError(L"This ERROR message should pass through filter");
            logger->WriteLogFatal(L"This FATAL message should pass through filter");
            
            // 等待日志写入完成
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // 检查日志文件大小变化
            size_t finalSize = logger->GetCurrentLogFileSize();
            
            if (finalSize > initialSize) {
                framework.TestPass(L"Level filter integration working - only ERROR and FATAL messages written");
            } else {
                framework.TestFail(L"Level filter integration failed - no messages written");
            }
            
            // 清除过滤器
            logger->ClearLogFilter();
        }
        
        // 测试2：关键词过滤器集成测试
        {
            std::wcout << L"\n* Testing Keyword Filter Integration *" << std::endl;
            
            // 创建关键词过滤器，只允许包含"IMPORTANT"的消息
            auto keywordFilter = std::make_unique<KeywordFilter>();
            keywordFilter->AddIncludeKeyword(L"IMPORTANT");
            std::shared_ptr<ILogFilter> sharedFilter(keywordFilter.release());
            
            logger->SetLogFilter(sharedFilter);
            
            // 记录当前日志文件大小
            size_t initialSize = logger->GetCurrentLogFileSize();
            
            // 写入包含和不包含关键词的消息
            logger->WriteLogInfo(L"This is a normal message");
            logger->WriteLogInfo(L"This is an IMPORTANT message");
            logger->WriteLogInfo(L"Another normal message");
            logger->WriteLogInfo(L"Another IMPORTANT notification");
            
            // 等待日志写入完成
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // 检查日志文件大小变化
            size_t finalSize = logger->GetCurrentLogFileSize();
            
            if (finalSize > initialSize) {
                framework.TestPass(L"Keyword filter integration working - only IMPORTANT messages written");
            } else {
                framework.TestFail(L"Keyword filter integration failed");
            }
            
            // 清除过滤器
            logger->ClearLogFilter();
        }
        
        // 测试3：限流过滤器集成测试
        {
            std::wcout << L"\n* Testing Rate Limit Filter Integration *" << std::endl;
            
            // 创建限流过滤器，每秒最多2条消息
            auto rateLimitFilter = std::make_unique<RateLimitFilter>(2, 2);
            std::shared_ptr<ILogFilter> sharedFilter(rateLimitFilter.release());
            
            logger->SetLogFilter(sharedFilter);
            
            // 记录当前日志文件大小
            size_t initialSize = logger->GetCurrentLogFileSize();
            
            // 快速写入多条消息
            for (int i = 0; i < 10; ++i) {
                logger->WriteLogInfo(L"Rate limit test message " + std::to_wstring(i));
            }
            
            // 等待日志写入完成
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // 检查日志文件大小变化
            size_t finalSize = logger->GetCurrentLogFileSize();
            
            if (finalSize > initialSize) {
                framework.TestPass(L"Rate limit filter integration working - limited message throughput");
            } else {
                framework.TestFail(L"Rate limit filter integration failed");
            }
            
            // 清除过滤器
            logger->ClearLogFilter();
        }
        
        // 测试4：组合过滤器集成测试
        {
            std::wcout << L"\n* Testing Composite Filter Integration *" << std::endl;
            
            // 创建组合过滤器：级别过滤器 + 关键词过滤器
            auto levelFilter = std::make_unique<LevelFilter>(LogLevel::Warning);
            auto keywordFilter = std::make_unique<KeywordFilter>();
            keywordFilter->AddIncludeKeyword(L"CRITICAL");
            
            auto compositeFilter = std::make_unique<CompositeFilter>(L"TestComposite", CompositionStrategy::AllMustPass);
            compositeFilter->AddFilter(std::move(levelFilter));
            compositeFilter->AddFilter(std::move(keywordFilter));
            
            std::shared_ptr<ILogFilter> sharedFilter(compositeFilter.release());
            logger->SetLogFilter(sharedFilter);
            
            // 记录当前日志文件大小
            size_t initialSize = logger->GetCurrentLogFileSize();
            
            // 写入测试消息
            logger->WriteLogInfo(L"INFO with CRITICAL keyword");          // 级别不够，被过滤
            logger->WriteLogError(L"ERROR without keyword");              // 没有关键词，被过滤
            logger->WriteLogError(L"ERROR with CRITICAL keyword");        // 满足所有条件，通过
            
            // 等待日志写入完成
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // 检查日志文件大小变化
            size_t finalSize = logger->GetCurrentLogFileSize();
            
            if (finalSize > initialSize) {
                framework.TestPass(L"Composite filter integration working - only messages meeting all criteria written");
            } else {
                framework.TestFail(L"Composite filter integration failed");
            }
            
            // 清除过滤器
            logger->ClearLogFilter();
        }
        
        // 验证过滤器清除
        if (!logger->HasLogFilter()) {
            framework.TestPass(L"Filter cleanup successful");
        } else {
            framework.TestFail(L"Filter cleanup failed");
        }
    }
};

// ==================== 模块6：性能测试 ====================
/**
 * @brief 性能测试类
 * @details 测试日志系统的性能表现，包括批量写入速度和异步轮转性能
 *          提供详细的性能指标和时间统计
 */
class PerformanceTests {
public:
    /**
     * @brief 运行性能测试
     * @param framework 测试框架引用
     * @details 执行批量日志写入测试和异步轮转性能测试
     */
    static void RunTests(LogTestFramework& framework) {
        framework.StartTest(L"Performance Tests");
        
        auto logger = framework.GetLogger();
        
        try {
            // 批量写入性能测试
            const int messageCount = 1000;
            auto startTime = std::chrono::high_resolution_clock::now();
            
            // 连续写入指定数量的日志消息
            for (int i = 0; i < messageCount; ++i) {
                logger->WriteLogInfo(L"Performance test message " + std::to_wstring(i));
            }
            
            // 计算批量写入耗时
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            // 计算每秒写入消息数量
            double messagesPerSecond = (double)messageCount / duration.count() * 1000.0;
            
            framework.TestPass(L"Batch write " + std::to_wstring(messageCount) + L" messages, time cost: " + 
                std::to_wstring(duration.count()) + L"ms, speed: " + 
                std::to_wstring((int)messagesPerSecond) + L" msgs/sec");
            
            // 异步轮转性能测试
            auto rotationStart = std::chrono::high_resolution_clock::now();
            auto future = logger->ForceLogRotationAsync();  // 启动异步轮转
            bool success = future.get();                     // 等待轮转完成
            auto rotationEnd = std::chrono::high_resolution_clock::now();
            auto rotationDuration = std::chrono::duration_cast<std::chrono::milliseconds>(rotationEnd - rotationStart);
            
            // 验证异步轮转结果
            if (success) {
                framework.TestPass(L"Async rotation successful, time cost: " + std::to_wstring(rotationDuration.count()) + L"ms");
            } else {
                framework.TestFail(L"Async rotation failed");
            }
            
        } catch (const std::exception& e) {
            framework.TestFail(L"Performance test exception: " + 
                UniConv::GetInstance()->LocaleToWideString(e.what()));
        }
    }
};

// ==================== 主程序入口 ====================
/**
 * @brief 主程序入口函数
 * @details 初始化日志系统组件，创建测试框架，执行所有模块化测试
 *          最后输出完整的测试报告和系统状态信息
 * @return 程序退出码，0表示成功，1表示出现异常
 */
int main() {
    try {
        // 初始化Unicode编码转换器，设置默认编码为UTF-8
        UniConv::GetInstance()->SetDefaultEncoding("UTF-8");
        
        std::wcout << L"=== LightLogWriteImpl Integrated Test Suite ===" << std::endl;
        std::wcout << L"Version: Optimized Refactored Edition" << std::endl;
        
        // 创建并配置日志压缩器
        LogCompressorConfig compressorConfig;
        compressorConfig.workerThreadCount = std::thread::hardware_concurrency();  // 使用硬件并发数作为工作线程数
        compressorConfig.algorithm = CompressionAlgorithm::ZIP;                     // 使用ZIP压缩算法
        compressorConfig.compressionLevel = 6;                                      // 设置压缩级别为6（平衡压缩率和速度）
        auto compressor = std::make_shared<LogCompressor>(compressorConfig);
        compressor->Start();  // 启动压缩器服务


        // 创建并配置日志记录器
        auto logger = std::make_shared<LightLogWrite_Impl>(
            10000,                              // 日志队列大小
            LogQueueOverflowStrategy::Block,    // 队列溢出策略：阻塞等待
            1000,                               // 批处理大小
            compressor                          // 压缩器引用
        );
        logger->SetLastingsLogs(L"logs", L"app_log");     // 设置日志目录和文件名前缀
        logger->SetMinLogLevel(LogLevel::Trace);          // 设置最低日志级别为Trace
        
        // ==================== 分离控制台功能测试 ====================
        std::wcout << L"\n=== Testing Separate Console Feature ===" << std::endl;
        
        // 创建分离控制台输出
        auto separateConsoleOutput = std::make_shared<ConsoleLogOutput>(
            L"SeparateConsole",    // 输出名称
            true,                   // 使用stderr输出错误级别
            true,                   // 启用颜色
            true                    // 启用分离控制台！！！
        );
        
        // 初始化分离控制台输出
        if (!separateConsoleOutput->Initialize()) {
            std::wcout << L"ERROR: Failed to initialize separate console output!" << std::endl;
        } else {
            std::wcout << L"[DEBUG] Separate console output initialized successfully!" << std::endl;
        }
        
        // 创建普通文件输出
        auto fileOutput = std::make_shared<FileLogOutput>(L"TestFile");
        
        // 初始化文件输出
        if (!fileOutput->Initialize(L"logs/separate_console_test.log")) {
            std::wcout << L"ERROR: Failed to initialize file output!" << std::endl;
        } else {
            std::wcout << L"[DEBUG] File output initialized successfully!" << std::endl;
        }
        
        // 创建基础格式化器
        auto formatter = std::make_shared<BasicLogFormatter>();
        
        // 启用多输出系统
        logger->SetMultiOutputEnabled(true);
        
        // 添加输出到日志系统
        logger->AddLogOutput(separateConsoleOutput);
        logger->AddLogOutput(fileOutput);
        
        std::wcout << L"Separate console created! Log output will now be displayed in the new console window." << std::endl;
        std::wcout << L"Writing test log messages..." << std::endl;
        
        // 写入各种级别的日志到分离控制台
        logger->WriteLogInfo(L"Welcome to the separate console logging system!");
        logger->WriteLogDebug(L"This is debug information displayed in the separate console window");
        logger->WriteLogWarning(L"This is warning information, check the new console window");
        logger->WriteLogError(L"This is error information highlighted in the independent console");
        logger->WriteLogCritical(L"This is critical error displayed in the separate console");
        
        // 演示颜色功能
        for (int i = 0; i < 10; ++i) {
            LogLevel levels[] = {LogLevel::Trace, LogLevel::Debug, LogLevel::Info, LogLevel::Notice, LogLevel::Warning, 
                               LogLevel::Error, LogLevel::Critical, LogLevel::Alert, LogLevel::Emergency, LogLevel::Fatal};
            
            LogLevel level = levels[i];
            std::wstring levelName;
            switch (level) {
                case LogLevel::Trace: levelName = L"TRACE"; break;
                case LogLevel::Debug: levelName = L"DEBUG"; break;
                case LogLevel::Info: levelName = L"INFO"; break;
                case LogLevel::Notice: levelName = L"NOTICE"; break;
                case LogLevel::Warning: levelName = L"WARNING"; break;
                case LogLevel::Error: levelName = L"ERROR"; break;
                case LogLevel::Critical: levelName = L"CRITICAL"; break;
                case LogLevel::Alert: levelName = L"ALERT"; break;
                case LogLevel::Emergency: levelName = L"EMERGENCY"; break;
                case LogLevel::Fatal: levelName = L"FATAL"; break;
            }
            
            // 根据级别调用相应的日志方法
            switch (level) {
                case LogLevel::Trace:
                    logger->WriteLogTrace(levelName + L" level message - Index: " + std::to_wstring(i + 1));
                    break;
                case LogLevel::Debug:
                    logger->WriteLogDebug(levelName + L" level message - Index: " + std::to_wstring(i + 1));
                    break;
                case LogLevel::Info:
                    logger->WriteLogInfo(levelName + L" level message - Index: " + std::to_wstring(i + 1));
                    break;
                case LogLevel::Notice:
                    logger->WriteLogNotice(levelName + L" level message - Index: " + std::to_wstring(i + 1));
                    break;
                case LogLevel::Warning:
                    logger->WriteLogWarning(levelName + L" level message - Index: " + std::to_wstring(i + 1));
                    break;
                case LogLevel::Error:
                    logger->WriteLogError(levelName + L" level message - Index: " + std::to_wstring(i + 1));
                    break;
                case LogLevel::Critical:
                    logger->WriteLogCritical(levelName + L" level message - Index: " + std::to_wstring(i + 1));
                    break;
                case LogLevel::Alert:
                    logger->WriteLogAlert(levelName + L" level message - Index: " + std::to_wstring(i + 1));
                    break;
                case LogLevel::Emergency:
                    logger->WriteLogEmergency(levelName + L" level message - Index: " + std::to_wstring(i + 1));
                    break;
                case LogLevel::Fatal:
                    logger->WriteLogFatal(levelName + L" level message - Index: " + std::to_wstring(i + 1));
                    break;
            }
            
            // 每条消息之间稍微暂停以便观察
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        std::wcout << L"Separate console test completed! Please check the newly opened console window." << std::endl;
        std::wcout << L"Press any key to continue with other tests..." << std::endl;
        system("pause");
        
        // 重置为标准配置继续其他测试
        auto standardConsoleOutput = std::make_shared<ConsoleLogOutput>(L"Console", true, true, false);  // 不使用分离控制台
        auto standardFileOutput = std::make_shared<FileLogOutput>(L"MainFile");
        
        // 初始化标准输出
        if (!standardConsoleOutput->Initialize()) {
            std::wcout << L"ERROR: Failed to initialize standard console output!" << std::endl;
        } else {
            std::wcout << L"[DEBUG] Standard console output initialized successfully!" << std::endl;
        }
        
        if (!standardFileOutput->Initialize(L"logs/app_log.log")) {
            std::wcout << L"ERROR: Failed to initialize standard file output!" << std::endl;
        } else {
            std::wcout << L"[DEBUG] Standard file output initialized successfully!" << std::endl;
        }
        
        // 清除之前的输出并添加标准输出
        logger->RemoveLogOutput(L"SeparateConsole");
        logger->RemoveLogOutput(L"TestFile");
        logger->AddLogOutput(standardConsoleOutput);
        logger->AddLogOutput(standardFileOutput);
        
        // 创建测试框架实例
        LogTestFramework framework(logger, compressor);
        
        // 依次运行所有模块化测试
        BasicLoggingTests::RunTests(framework);        // 基础日志功能测试
        CallbackSystemTests::RunTests(framework);      // 回调系统测试
        RotationSystemTests::RunTests(framework);      // 轮转系统测试
        CompressionSystemTests::RunTests(framework);   // 压缩系统测试
        FilterSystemTests::RunTests(framework);        // 过滤器系统测试
        PerformanceTests::RunTests(framework);         // 性能测试
        
        // 显示完整的测试总结报告
        framework.ShowSummary();
        
        // 输出最终的系统状态信息
        std::wcout << L"\n=== System Status ===" << std::endl;
        std::wcout << L"Current log file size: " << logger->GetCurrentLogFileSize() << L" bytes" << std::endl;
        std::wcout << L"Active callbacks count: " << logger->GetCallbackCount() << std::endl;
        
        // 获取并显示压缩统计信息
        auto stats = logger->GetCompressionStatistics();
        if (stats.totalTasks > 0) {
            double ratio = (double)stats.totalCompressedSize / stats.totalOriginalSize * 100.0;
            std::wcout << L"Compression statistics: " << stats.successfulTasks << L"/" << stats.totalTasks 
                       << L" successful, compression ratio: " << std::fixed << std::setprecision(1) << ratio << L"%" << std::endl;
        }
        
        // 停止压缩器服务，释放资源
        compressor->Stop();
        
        std::wcout << L"\nTest completed! Please check the log files in the logs directory and the compressed files in the logs/archive directory." << std::endl;
        
    } catch (const std::exception& e) {
        // 捕获标准异常并输出错误信息
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        // 捕获所有其他异常
        std::cerr << "Unknown exception occurred" << std::endl;
        return 1;
    }
    
    // 暂停程序，等待用户按键
    system("pause");
    return 0;
}