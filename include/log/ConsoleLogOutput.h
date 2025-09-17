#pragma once

#include "BaseLogOutput.h"
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * @brief Console output implementation
 * @details Writes logs to console (stdout/stderr) with optional color support
 *  Supports creating a separate console window with dedicated thread
 */
class ConsoleLogOutput : public BaseLogOutput {
public:
    // Thread-safe queue for console output
    struct ConsoleLogItem {
        std::wstring formattedLog;
        LogLevel level;
        std::chrono::system_clock::time_point timestamp;

        // 默认构造函数
        ConsoleLogItem() : level(LogLevel::Info), timestamp(std::chrono::system_clock::now()) {}
        // 带参数的构造函数
        ConsoleLogItem(const std::wstring& log, LogLevel lvl)
            : formattedLog(log), level(lvl), timestamp(std::chrono::system_clock::now()) {
        }
    };
private:
    bool m_useStderr;           // Use stderr instead of stdout for error levels
    bool m_enableColors;        // Enable ANSI color codes
    LogLevel m_stderrThreshold; // Minimum level to use stderr
    
    // Separate console support
    bool m_useSeparateConsole;  // Whether to use separate console window
    std::atomic<bool> m_consoleThreadRunning; // Console thread running flag
    std::thread m_consoleThread;              // Dedicated console thread
    
    // Process and pipe handles for separate console process
#ifdef _WIN32
    HANDLE m_consoleProcess;    // Handle to separate console process
    HANDLE m_consolePipeWrite;  // Write end of pipe to console process
    HANDLE m_consolePipeRead;   // Read end of pipe (used by console process)
#else
    void* m_consoleProcess;     // Placeholder for non-Windows platforms
    void* m_consolePipeWrite;   // Placeholder for non-Windows platforms
    void* m_consolePipeRead;    // Placeholder for non-Windows platforms
#endif
    
    std::queue<ConsoleLogItem> m_logQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    std::atomic<bool> m_shutdownRequested;

public:
    /**
     * @brief Constructor
     * @param outputName Name of this console output
     * @param useStderr Whether to use stderr for error levels
     * @param enableColors Whether to enable color output
     * @param useSeparateConsole Whether to create a separate console window
     */
    ConsoleLogOutput(const std::wstring& outputName = L"Console",
                     bool useStderr = true,
                     bool enableColors = true,
                     bool useSeparateConsole = false);

    /**
     * @brief Destructor
     */
    virtual ~ConsoleLogOutput();

    // Configuration methods
    void SetUseStderr(bool useStderr) { m_useStderr = useStderr; }
    bool GetUseStderr() const { return m_useStderr; }
    
    void SetEnableColors(bool enableColors) { m_enableColors = enableColors; }
    bool GetEnableColors() const { return m_enableColors; }
    
    void SetStderrThreshold(LogLevel threshold) { m_stderrThreshold = threshold; }
    LogLevel GetStderrThreshold() const { return m_stderrThreshold; }
    
    void SetUseSeparateConsole(bool useSeparateConsole);
    bool GetUseSeparateConsole() const { return m_useSeparateConsole; }

protected:
    // BaseLogOutput virtual methods implementation
    LogOutputResult WriteLogInternal(const std::wstring& formattedLog, const LogCallbackInfo& originalInfo) override;
    void FlushInternal() override;
    bool IsAvailableInternal() const override;
    bool InitializeInternal(const std::wstring& config) override;
    void ShutdownInternal() override;
    std::wstring GetConfigStringInternal() const override;

private:
    // Helper methods
    std::string GetColorCode(LogLevel level) const;
    std::string GetResetColorCode() const;
    bool ShouldUseStderr(LogLevel level) const;
    std::string WStringToString(const std::wstring& wstr) const;
    std::string GetLogLevelPrefix(LogLevel level) const;  // 获取日志级别前缀，如 [DEBUG]、[INFO] 等
    
    // Separate console management methods
    void InitializeSeparateConsole();
    void ShutdownSeparateConsole();
    void ConsoleThreadProc();  // Thread procedure for console output
    
    // Queue management
    void EnqueueLogItem(const ConsoleLogItem& item);
    bool DequeueLogItem(ConsoleLogItem& item);
};