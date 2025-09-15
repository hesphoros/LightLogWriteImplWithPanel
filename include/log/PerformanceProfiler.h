#pragma once
#include <chrono>
#include <string>
#include <iostream>
#include <iomanip>

/**
 * 简单的性能测量工具类
 * 用于测量代码块的执行时间
 */
class PerformanceProfiler {
private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::string operationName;
    bool isRunning;

public:
    explicit PerformanceProfiler(const std::string& name) 
        : operationName(name), isRunning(true) {
        startTime = std::chrono::high_resolution_clock::now();
    }

    ~PerformanceProfiler() {
        if (isRunning) {
            Stop();
        }
    }

    void Stop() {
        if (isRunning) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                endTime - startTime).count();
            
            std::wcout << L"[PERF] " << operationName.c_str() 
                      << L": " << duration << L" µs (" 
                      << std::fixed << std::setprecision(3) 
                      << duration / 1000.0 << L" ms)" << std::endl;
            
            isRunning = false;
        }
    }

    // 获取当前运行时间（微秒）
    long long GetElapsedMicroseconds() const {
        if (isRunning) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(
                currentTime - startTime).count();
        }
        return 0;
    }
};

// 便捷的宏定义
#define PROFILE_FUNCTION() PerformanceProfiler prof(__FUNCTION__)
#define PROFILE_SCOPE(name) PerformanceProfiler prof(name)
#define PROFILE_BLOCK(name) for(PerformanceProfiler prof(name); prof.GetElapsedMicroseconds() >= 0; prof.Stop())