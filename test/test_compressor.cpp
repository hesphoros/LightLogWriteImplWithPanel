#include "log/LogCompressor.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    try {
        std::wcout << L"Testing LogCompressor with BS::thread_pool..." << std::endl;
        
        // 创建配置
        LogCompressorConfig config;
        config.workerThreadCount = 4;  // 使用4个工作线程
        config.enableStatistics = true;
        
        // 创建压缩器
        LogCompressor compressor(config);
        
        // 启动压缩器
        compressor.Start();
        
        // 显示状态
        std::wcout << L"Compressor Status:" << std::endl;
        std::wcout << compressor.GetStatusInfo() << std::endl;
        
        std::wcout << L"Is Compressing: " << (compressor.IsCompressing() ? L"Yes" : L"No") << std::endl;
        std::wcout << L"Active Tasks: " << compressor.GetActiveTasksCount() << std::endl;
        std::wcout << L"Pending Tasks: " << compressor.GetPendingTasksCount() << std::endl;
        
        // 测试异步压缩（如果有测试文件的话）
        // compressor.CompressAsync(L"test.log", L"test.zip", [](bool success) {
        //     std::wcout << L"Compression result: " << (success ? L"Success" : L"Failed") << std::endl;
        // });
        
        // 等待一段时间
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // 停止压缩器
        compressor.Stop();
        
        // 获取统计信息
        auto stats = compressor.GetStatistics();
        std::wcout << L"Final Statistics:" << std::endl;
        std::wcout << L"  Total Tasks: " << stats.totalTasks << std::endl;
        std::wcout << L"  Successful Tasks: " << stats.successfulTasks << std::endl;
        std::wcout << L"  Failed Tasks: " << stats.failedTasks << std::endl;
        
        std::wcout << L"Test completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::wcerr << L"Error: " << e.what() << std::endl;
        return 1;
    }
}