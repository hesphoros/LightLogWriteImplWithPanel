#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <filesystem>

#include <UniConv.h>

namespace fs = std::filesystem;
// 引入我们的日志系统头文件
#include "include/log/LightLogWriteImpl.h"
#include "include/log/LogOutputManager.h"
#include "include/log/ConsoleLogOutput.h"
#include "include/log/FileLogOutput.h"
#include "include/log/BasicLogFormatter.h"


// Check compressed files in archive directory
void CheckArchiveDirectory(const std::wstring& archivePath) {
    try {
        if (!fs::exists(archivePath)) {
            std::wcout << L"Archive directory does not exist: " << archivePath << std::endl;
            return;
        }
        
        std::wcout << L"\n=== Archive Directory Check ===" << std::endl;
        std::wcout << L"Path: " << archivePath << std::endl;
        
        int fileCount = 0;
        size_t totalSize = 0;
        
        for (const auto& entry : fs::directory_iterator(archivePath)) {
            if (fs::is_regular_file(entry.status())) {
                fileCount++;
                size_t fileSize = fs::file_size(entry.path());
                totalSize += fileSize;
                
                std::wcout << L"Archive file: " << entry.path().filename().wstring() 
                          << L" (Size: " << fileSize << L" bytes)" << std::endl;
                
                // Check if it's a ZIP compressed file
                if (entry.path().extension() == L".zip") {
                    std::wcout << L"  -> ZIP compressed archive" << std::endl;
                } else if (entry.path().extension() == L".log") {
                    std::wcout << L"  -> Uncompressed log file" << std::endl;
                }
            }
        }
        
        std::wcout << L"Total: " << fileCount << L" archive files, using " << totalSize << L" bytes" << std::endl;
        
    } catch (const std::exception& e) {
        std::wcerr << L"Error checking archive directory: " 
                  << UniConv::GetInstance()->LocaleToWideString(e.what()) << std::endl;
    }
}

// Display archive compression statistics
void ShowArchiveCompressionStats(const std::wstring& archivePath) {
    try {
        std::wcout << L"\n=== ZIP Compression Statistics ===" << std::endl;
        
        if (!fs::exists(archivePath)) {
            std::wcout << L"Archive directory does not exist." << std::endl;
            return;
        }
        
        int compressedFiles = 0;
        int uncompressedFiles = 0;
        size_t totalCompressedSize = 0;
        size_t totalUncompressedSize = 0;
        
        for (const auto& entry : fs::directory_iterator(archivePath)) {
            if (fs::is_regular_file(entry.status())) {
                std::wstring filename = entry.path().filename().wstring();
                size_t fileSize = fs::file_size(entry.path());
                
                if (entry.path().extension() == L".zip") {
                    compressedFiles++;
                    totalCompressedSize += fileSize;
                    std::wcout << L"ZIP compressed file: " << filename << L" (" << fileSize << L" bytes)" << std::endl;
                } else {
                    uncompressedFiles++;
                    totalUncompressedSize += fileSize;
                    std::wcout << L"Uncompressed file: " << filename << L" (" << fileSize << L" bytes)" << std::endl;
                }
            }
        }
        
        std::wcout << L"\nSummary:" << std::endl;
        std::wcout << L"ZIP compressed files: " << compressedFiles << L" (Total size: " << totalCompressedSize << L" bytes)" << std::endl;
        std::wcout << L"Uncompressed files: " << uncompressedFiles << L" (Total size: " << totalUncompressedSize << L" bytes)" << std::endl;
        
        if (totalUncompressedSize > 0 && totalCompressedSize > 0) {
            double compressionRatio = (double)totalCompressedSize / (totalCompressedSize + totalUncompressedSize) * 100.0;
            std::wcout << L"Compressed files ratio: " << std::fixed << std::setprecision(2) << compressionRatio << L"%" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::wcerr << L"Error displaying compression statistics: " 
                  << UniConv::GetInstance()->LocaleToWideString(e.what()) << std::endl;
    }
}

int main() {
    try {
        // Set console to UTF-8 encoding
        
        
        // Initialize encoding converter
        UniConv::GetInstance()->SetDefaultEncoding("UTF-8");

        // Create logger object
        LightLogWrite_Impl logger;
        logger.SetLastingsLogs(L"logs", L"app_log");

        // Set minimum log level to Trace
        logger.SetMinLogLevel(LogLevel::Trace);

        std::wcout << L"=== LightLogWriteImpl ZIP Compression Log Test ===" << std::endl;

        // Test 1: Basic logging functionality
        std::wcout << L"\n=== Test 1: Basic Logging Functionality ===" << std::endl;

        logger.WriteLogInfo(L"Basic info log test - ZIP compression feature");
        logger.WriteLogError(L"Basic error log test - ZIP compression feature");
        logger.WriteLogCritical(L"Basic critical log test - ZIP compression feature");
        logger.WriteLogTrace(L"Basic trace log test - ZIP compression feature");
        logger.WriteLogDebug(L"Basic debug log test - ZIP compression feature");
        logger.WriteLogNotice(L"Basic notice log test - ZIP compression feature");
        logger.WriteLogWarning(L"Basic warning log test - ZIP compression feature");
        logger.WriteLogAlert(L"Basic alert log test - ZIP compression feature");
        logger.WriteLogEmergency(L"Basic emergency log test - ZIP compression feature");
        logger.WriteLogFatal(L"Basic fatal log test - ZIP compression feature");

        std::wcout << L"Basic logging test completed!" << std::endl;

        // Test 2: Callback functionality
        std::wcout << L"\n=== Test 2: Callback Functionality ===" << std::endl;

        int callbackCount = 0;
        auto simpleCallback = [&callbackCount](const LogCallbackInfo& info) {
            callbackCount++;
            std::wcout << L"[CALLBACK] " << info.levelString << L" : " << info.message << std::endl;
            };

        CallbackHandle handle = logger.SubscribeToLogEvents(simpleCallback);
        std::wcout << L"Registered callbacks count: " << logger.GetCallbackCount() << std::endl;

        logger.WriteLogInfo(L"Callback test info 1 - Will trigger callback function");
        logger.WriteLogError(L"Callback test error 2 - Will trigger callback function");

        // Test 3: Log rotation configuration with ZIP compression
        std::wcout << L"\n=== Test 3: Log Rotation Configuration (ZIP Compression) ===" << std::endl;

        LogRotationConfig config;
        config.strategy = LogRotationStrategy::SizeAndTime;
        config.maxFileSizeMB = 1024;  // MB for quick testing
        config.timeInterval = TimeRotationInterval::Daily;
        config.enableCompression = true;  // Enable ZIP compression
        config.archiveDirectory = L"logs/archive";
        config.maxArchiveFiles = 50;

        logger.SetLogRotationConfig(config);
        std::wcout << L"Rotation configuration set: Max 1MB, ZIP compression enabled, max 5 archive files retained" << std::endl;

        // Display current configuration
        auto currentConfig = logger.GetLogRotationConfig();
        std::wcout << L"Current config - Max file size: " << currentConfig.maxFileSizeMB << L" MB" << std::endl;
        std::wcout << L"Archive directory: " << currentConfig.archiveDirectory << std::endl;
        
        if (currentConfig.enableCompression) {
            std::wcout << L"Log compression: Enabled (using miniz ZIP format)" << std::endl;
        } else {
            std::wcout << L"Log compression: Disabled" << std::endl;
        }

        // Test 4: Generate large number of logs to trigger rotation
        std::wcout << L"\n=== Test 4: Generate Large Number of Logs with Performance Analysis ===" << std::endl;
		
        logger.ClearAllLogCallbacks();
        
        // 批量日志写入
        for (int i = 0; i < 1000; ++i) {
            logger.WriteLogInfo(L"Batch log test " + std::to_wstring(i) +
                L" - This is a long log message to quickly reach file size limit and trigger rotation. Adding more text to increase file size and ensure rotation occurs. ZIP compression test content.");

            // 每10万条消息显示一次性能统计
            if (i % 100 == 0 && i > 0) {
                size_t currentSize = logger.GetCurrentLogFileSize();
                std::wcout << L"Progress: " << i << L" messages, file size: " << currentSize << L" bytes" << std::endl;
            }
        }
        
        std::wcout << L"Batch logging completed. Analyzing compression queue performance..." << std::endl;

        // Test 5: Manual log rotation with performance analysis
        std::wcout << L"\n=== Test 5: Manual Log Rotation Performance Analysis ===" << std::endl;
        size_t sizeBeforeRotation = logger.GetCurrentLogFileSize();
        std::wcout << L"File size before rotation: " << sizeBeforeRotation << L" bytes" << std::endl;

        try {
            // Brief pause to ensure any pending writes are queued
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            std::wcout << L"Starting manual rotation..." << std::endl;
            
            logger.ForceLogRotation();

            size_t sizeAfterRotation = logger.GetCurrentLogFileSize();
            std::wcout << L"File size after rotation: " << sizeAfterRotation << L" bytes" << std::endl;
            
            CheckArchiveDirectory(L"logs/archive");
            
        }
        catch (const std::exception& e) {
            std::wcerr << L"Error during manual rotation: " << UniConv::GetInstance()->LocaleToWideString(e.what()) << std::endl;
            std::wcout << L"Continuing with remaining tests..." << std::endl;
        }

        // Test 6: Verify new file logging
        std::wcout << L"\n=== Test 6: Verify New File Logging ===" << std::endl;

        try {
            // No delay needed - rotation creates new file immediately
            logger.WriteLogInfo(L"First info message after rotation");
            logger.WriteLogWarning(L"Warning message after rotation");
            logger.WriteLogError(L"Error message after rotation");
            
            std::wcout << L"Successfully wrote messages after rotation" << std::endl;
        }
        catch (const std::exception& e) {
            std::wcerr << L"Error writing after rotation: " << UniConv::GetInstance()->LocaleToWideString(e.what()) << std::endl;
        }

        // Final status display
        std::wcout << L"\n=== Final Status ===" << std::endl;
        std::wcout << L"Active callback count: " << logger.GetCallbackCount() << std::endl;
        std::wcout << L"Current log file size: " << logger.GetCurrentLogFileSize() << L" bytes" << std::endl;
        std::wcout << L"Total callback invocations: " << callbackCount << std::endl;

        // Clean up callback
        logger.UnsubscribeFromLogEvents(handle);
        std::wcout << L"Callback count after cleanup: " << logger.GetCallbackCount() << std::endl;

        std::wcout << L"\n=== Test Completed ===" << std::endl;
        std::wcout << L"Please check log files in ./logs directory" << std::endl;
        std::wcout << L"Please check ZIP archive files in ./logs/archive directory" << std::endl;
        
        // Display ZIP compression statistics
        ShowArchiveCompressionStats(L"logs/archive");

        // Brief wait to allow async compression queue to process any remaining tasks
        std::wcout << L"Allowing async compression queue to finish..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));

    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        return 1;
    }
	system("pause");
    return 0;
}