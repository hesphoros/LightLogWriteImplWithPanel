#pragma once
#define _CRT_SECURE_NO_WARNINGS

/**
 * @file DebugUtils.h
 * @brief 统一的调试工具和宏定义
 * @author LightLog Team
 * @date 2025-09-17
 *
 * 这个文件提供了项目中统一的调试和日志输出控制机制。
 * 通过编译时宏定义可以控制调试信息的输出，避免在生产环境中产生不必要的输出。
 */

#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <iomanip>

// 调试级别定义
#define DEBUG_LEVEL_NONE    0   // 无调试输出
#define DEBUG_LEVEL_ERROR   1   // 仅错误信息
#define DEBUG_LEVEL_WARNING 2   // 错误和警告信息
#define DEBUG_LEVEL_INFO    3   // 错误、警告和信息
#define DEBUG_LEVEL_VERBOSE 4   // 所有调试信息

// 默认调试级别 - 在发布版本中应设置为 DEBUG_LEVEL_NONE
#ifndef LIGHTLOG_DEBUG_LEVEL
    #ifdef _DEBUG
        #define LIGHTLOG_DEBUG_LEVEL DEBUG_LEVEL_INFO
    #else
        #define LIGHTLOG_DEBUG_LEVEL DEBUG_LEVEL_NONE
    #endif
#endif

// 启用/禁用特定模块的调试输出
#ifndef LIGHTLOG_DEBUG_MULTIOUTPUT
    #define LIGHTLOG_DEBUG_MULTIOUTPUT 1
#endif

#ifndef LIGHTLOG_DEBUG_CONSOLE
    #define LIGHTLOG_DEBUG_CONSOLE 1
#endif

#ifndef LIGHTLOG_DEBUG_ROTATION
    #define LIGHTLOG_DEBUG_ROTATION 1
#endif

#ifndef LIGHTLOG_DEBUG_COMPRESSION
    #define LIGHTLOG_DEBUG_COMPRESSION 1
#endif

namespace LightLog {
namespace Debug {

/**
 * @brief 获取当前时间戳字符串
 * @return 格式化的时间戳字符串
 */
inline std::wstring GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::wstringstream ss;
    ss << std::put_time(std::localtime(&time_t), L"%H:%M:%S");
    ss << L"." << std::setfill(L'0') << std::setw(3) << ms.count();
    return ss.str();
}

/**
 * @brief 获取当前线程ID字符串
 * @return 线程ID字符串
 */
inline std::wstring GetThreadId() {
    std::wstringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

/**
 * @brief 格式化调试消息
 * @param level 调试级别字符串
 * @param module 模块名称
 * @param message 消息内容
 * @return 格式化后的完整调试消息
 */
inline std::wstring FormatDebugMessage(const std::wstring& level, 
                                     const std::wstring& module, 
                                     const std::wstring& message) {
    std::wstringstream ss;
    ss << L"[" << GetTimestamp() << L"][" << level << L"][" << module 
       << L"][T:" << GetThreadId() << L"] " << message;
    return ss.str();
}

} // namespace Debug
} // namespace LightLog

// 基础调试宏
#if LIGHTLOG_DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
    #define LIGHTLOG_DEBUG_ERROR(module, message) \
        do { \
            std::wcout << LightLog::Debug::FormatDebugMessage(L"ERROR", L#module, message) << std::endl; \
        } while(0)
#else
    #define LIGHTLOG_DEBUG_ERROR(module, message) do { } while(0)
#endif

#if LIGHTLOG_DEBUG_LEVEL >= DEBUG_LEVEL_WARNING
    #define LIGHTLOG_DEBUG_WARNING(module, message) \
        do { \
            std::wcout << LightLog::Debug::FormatDebugMessage(L"WARN", L#module, message) << std::endl; \
        } while(0)
#else
    #define LIGHTLOG_DEBUG_WARNING(module, message) do { } while(0)
#endif

#if LIGHTLOG_DEBUG_LEVEL >= DEBUG_LEVEL_INFO
    #define LIGHTLOG_DEBUG_INFO(module, message) \
        do { \
            std::wcout << LightLog::Debug::FormatDebugMessage(L"INFO", L#module, message) << std::endl; \
        } while(0)
#else
    #define LIGHTLOG_DEBUG_INFO(module, message) do { } while(0)
#endif

#if LIGHTLOG_DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE
    #define LIGHTLOG_DEBUG_VERBOSE(module, message) \
        do { \
            std::wcout << LightLog::Debug::FormatDebugMessage(L"VERBOSE", L#module, message) << std::endl; \
        } while(0)
#else
    #define LIGHTLOG_DEBUG_VERBOSE(module, message) do { } while(0)
#endif

// 模块特定的调试宏
#if LIGHTLOG_DEBUG_MULTIOUTPUT
    #define LIGHTLOG_DEBUG_MULTIOUTPUT_INFO(message) LIGHTLOG_DEBUG_INFO(MultiOutput, message)
    #define LIGHTLOG_DEBUG_MULTIOUTPUT_VERBOSE(message) LIGHTLOG_DEBUG_VERBOSE(MultiOutput, message)
    #define LIGHTLOG_DEBUG_MULTIOUTPUT_ERROR(message) LIGHTLOG_DEBUG_ERROR(MultiOutput, message)
#else
    #define LIGHTLOG_DEBUG_MULTIOUTPUT_INFO(message) do { } while(0)
    #define LIGHTLOG_DEBUG_MULTIOUTPUT_VERBOSE(message) do { } while(0)
    #define LIGHTLOG_DEBUG_MULTIOUTPUT_ERROR(message) do { } while(0)
#endif

#if LIGHTLOG_DEBUG_CONSOLE
    #define LIGHTLOG_DEBUG_CONSOLE_INFO(message) LIGHTLOG_DEBUG_INFO(Console, message)
    #define LIGHTLOG_DEBUG_CONSOLE_VERBOSE(message) LIGHTLOG_DEBUG_VERBOSE(Console, message)
    #define LIGHTLOG_DEBUG_CONSOLE_ERROR(message) LIGHTLOG_DEBUG_ERROR(Console, message)
#else
    #define LIGHTLOG_DEBUG_CONSOLE_INFO(message) do { } while(0)
    #define LIGHTLOG_DEBUG_CONSOLE_VERBOSE(message) do { } while(0)
    #define LIGHTLOG_DEBUG_CONSOLE_ERROR(message) do { } while(0)
#endif

#if LIGHTLOG_DEBUG_ROTATION
    #define LIGHTLOG_DEBUG_ROTATION_INFO(message) LIGHTLOG_DEBUG_INFO(Rotation, message)
    #define LIGHTLOG_DEBUG_ROTATION_VERBOSE(message) LIGHTLOG_DEBUG_VERBOSE(Rotation, message)
    #define LIGHTLOG_DEBUG_ROTATION_ERROR(message) LIGHTLOG_DEBUG_ERROR(Rotation, message)
#else
    #define LIGHTLOG_DEBUG_ROTATION_INFO(message) do { } while(0)
    #define LIGHTLOG_DEBUG_ROTATION_VERBOSE(message) do { } while(0)
    #define LIGHTLOG_DEBUG_ROTATION_ERROR(message) do { } while(0)
#endif

#if LIGHTLOG_DEBUG_COMPRESSION
    #define LIGHTLOG_DEBUG_COMPRESSION_INFO(message) LIGHTLOG_DEBUG_INFO(Compression, message)
    #define LIGHTLOG_DEBUG_COMPRESSION_VERBOSE(message) LIGHTLOG_DEBUG_VERBOSE(Compression, message)
    #define LIGHTLOG_DEBUG_COMPRESSION_ERROR(message) LIGHTLOG_DEBUG_ERROR(Compression, message)
#else
    #define LIGHTLOG_DEBUG_COMPRESSION_INFO(message) do { } while(0)
    #define LIGHTLOG_DEBUG_COMPRESSION_VERBOSE(message) do { } while(0)
    #define LIGHTLOG_DEBUG_COMPRESSION_ERROR(message) do { } while(0)
#endif

// 便捷宏 - 支持流式输出
#define LIGHTLOG_DEBUG_STREAM(level, module) \
    if (LIGHTLOG_DEBUG_LEVEL >= level) { \
        std::wstringstream __debug_ss; \
        __debug_ss

#define LIGHTLOG_DEBUG_STREAM_END(level, module) \
        ; std::wcout << LightLog::Debug::FormatDebugMessage(L#level, L#module, __debug_ss.str()) << std::endl; \
    } else (void)0

// 流式调试宏使用示例：
// LIGHTLOG_DEBUG_STREAM(DEBUG_LEVEL_INFO, MultiOutput) << L"Processing item: " << itemId LIGHTLOG_DEBUG_STREAM_END(INFO, MultiOutput);

// 条件调试宏
#define LIGHTLOG_DEBUG_IF(condition, level, module, message) \
    do { \
        if (condition) { \
            LIGHTLOG_DEBUG_##level(module, message); \
        } \
    } while(0)

// 性能调试宏
#if LIGHTLOG_DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE
    #define LIGHTLOG_DEBUG_PERFORMANCE_START(name) \
        auto __perf_start_##name = std::chrono::high_resolution_clock::now()
    
    #define LIGHTLOG_DEBUG_PERFORMANCE_END(name, module) \
        do { \
            auto __perf_end = std::chrono::high_resolution_clock::now(); \
            auto __perf_duration = std::chrono::duration_cast<std::chrono::microseconds>(__perf_end - __perf_start_##name); \
            std::wstringstream __perf_ss; \
            __perf_ss << L"Performance[" << L#name << L"]: " << __perf_duration.count() << L" microseconds"; \
            LIGHTLOG_DEBUG_VERBOSE(module, __perf_ss.str()); \
        } while(0)
#else
    #define LIGHTLOG_DEBUG_PERFORMANCE_START(name) do { } while(0)
    #define LIGHTLOG_DEBUG_PERFORMANCE_END(name, module) do { } while(0)
#endif