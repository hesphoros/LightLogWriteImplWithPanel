#include "log/LightLogWriteImpl.h"
#include "log/LogCompressor.h"
#include "log/ConsoleLogOutput.h"
#include "log/FileLogOutput.h"
#include "log/LogFilters.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    try {
        std::wcout << L"=== LightLog Example Application ===" << std::endl;
        
        // 1. Create compressor configuration
        LogCompressorConfig compressorConfig;
        compressorConfig.algorithm = CompressionAlgorithm::ZIP;
        compressorConfig.compressionLevel = 6;
        compressorConfig.workerThreadCount = 2;
        
        // 2. Create compressor
        auto compressor = std::make_shared<LogCompressor>(compressorConfig);
        compressor->Start();
        
        // 3. Create logger with configuration
        auto logger = std::make_shared<LightLogWrite_Impl>(
            10000,                              // Queue size
            LogQueueOverflowStrategy::Block,    // Overflow strategy
            1000,                               // Batch size
            compressor                          // Compressor
        );
        
        // 4. Configure log files
        logger->SetLastingsLogs(L"logs", L"example_app");
        logger->SetMinLogLevel(LogLevel::Info);
        
        // 5. Configure rotation
        LogRotationConfig rotationConfig;
        rotationConfig.strategy = LogRotationStrategy::SizeAndTime;
        rotationConfig.maxFileSizeMB = 10;  // Rotate at 10MB
        rotationConfig.enableCompression = true;
        rotationConfig.archiveDirectory = L"logs/archive";
        logger->SetLogRotationConfig(rotationConfig);
        
        // 6. Enable multi-output system
        logger->SetMultiOutputEnabled(true);
        
        // 7. Add console output
        auto consoleOutput = std::make_shared<ConsoleLogOutput>(
            L"Console", true, true, false);  // name, stderr, colors, separate console
        logger->AddLogOutput(consoleOutput);
        
        // 8. Add file output
        auto fileOutput = std::make_shared<FileLogOutput>(L"ExampleFile");
        fileOutput->Initialize(L"logs/example_detailed.log");
        logger->AddLogOutput(fileOutput);
        
        // 9. Set up a filter (only warnings and above)
        auto levelFilter = std::make_unique<LevelFilter>(LogLevel::Warning);
        logger->SetLogFilter(std::move(levelFilter));
        
        std::wcout << L"Logger configured. Writing test messages..." << std::endl;
        
        // 10. Write various log messages
        logger->WriteLogInfo(L"Application started successfully");
        logger->WriteLogInfo(L"This INFO message should be filtered out");
        logger->WriteLogWarning(L"This is a warning message");
        logger->WriteLogError(L"This is an error message");
        logger->WriteLogCritical(L"This is a critical message");
        
        // 11. Demonstrate callback system
        int callbackCount = 0;
        auto callback = [&callbackCount](const LogCallbackInfo& info) {
            callbackCount++;
            std::wcout << L"[CALLBACK] " << info.levelString << L": " << info.message << std::endl;
        };
        
        auto handle = logger->SubscribeToLogEvents(callback);
        
        // Write more messages to trigger callbacks
        logger->WriteLogWarning(L"Callback test message 1");
        logger->WriteLogError(L"Callback test message 2");
        
        // Wait for async processing
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // 12. Force log rotation for demonstration
        std::wcout << L"Forcing log rotation..." << std::endl;
        logger->ForceLogRotation();
        
        // 13. Write some more messages after rotation
        logger->WriteLogWarning(L"Message after rotation");
        logger->WriteLogError(L"Another message after rotation");
        
        // 14. Display statistics
        auto stats = logger->GetCompressionStatistics();
        if (stats.totalTasks > 0) {
            double ratio = (double)stats.totalCompressedSize / stats.totalOriginalSize * 100.0;
            std::wcout << L"Compression statistics: " << stats.successfulTasks << L"/" 
                       << stats.totalTasks << L" successful, compression ratio: " 
                       << ratio << L"%" << std::endl;
        }
        
        std::wcout << L"Callback count: " << callbackCount << std::endl;
        std::wcout << L"Current log file size: " << logger->GetCurrentLogFileSize() << L" bytes" << std::endl;
        
        // 15. Cleanup
        logger->UnsubscribeFromLogEvents(handle);
        logger->ClearLogFilter();
        compressor->Stop();
        
        std::wcout << L"Example completed successfully!" << std::endl;
        std::wcout << L"Check the 'logs' directory for generated files." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        return 1;
    }
    
    return 0;
}