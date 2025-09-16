#include "../../include/log/ConsoleLogOutput.h"
#include "../../include/log/UniConv.h"
#include "../../include/log/DebugUtils.h"
#include <iostream>
#include <map>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#endif

ConsoleLogOutput::ConsoleLogOutput(const std::wstring& outputName, bool useStderr, bool enableColors, bool useSeparateConsole)
    : BaseLogOutput(outputName, L"Console")
    , m_useStderr(useStderr)
    , m_enableColors(enableColors)
    , m_stderrThreshold(LogLevel::Warning)
    , m_useSeparateConsole(useSeparateConsole)
    , m_consoleThreadRunning(false)
    , m_shutdownRequested(false)
#ifdef _WIN32
    , m_consoleProcess(INVALID_HANDLE_VALUE)
    , m_consolePipeWrite(INVALID_HANDLE_VALUE)
    , m_consolePipeRead(INVALID_HANDLE_VALUE)
#else
    , m_consoleProcess(nullptr)
    , m_consolePipeWrite(nullptr)
    , m_consolePipeRead(nullptr)
#endif
{
    if (m_useSeparateConsole) {
        InitializeSeparateConsole();
    }
}

ConsoleLogOutput::~ConsoleLogOutput() {
    if (m_useSeparateConsole) {
        ShutdownSeparateConsole();
    }
}

LogOutputResult ConsoleLogOutput::WriteLogInternal(const std::wstring& formattedLog, const LogCallbackInfo& originalInfo) {
    try {
        // 调试信息：检查分离控制台状态
        LIGHTLOG_DEBUG_STREAM(DEBUG_LEVEL_VERBOSE, Console) << L"WriteLogInternal called, m_useSeparateConsole = " << (m_useSeparateConsole ? L"true" : L"false") LIGHTLOG_DEBUG_STREAM_END(VERBOSE, Console);
        
        if (m_useSeparateConsole) {
            // Use separate console with thread-safe queue
            LIGHTLOG_DEBUG_CONSOLE_VERBOSE(L"Using separate console, creating log item...");
            ConsoleLogItem item(formattedLog, originalInfo.level);
            
            EnqueueLogItem(item);
            return LogOutputResult::Success;
        }
        else {
            // Original implementation for current console
            std::string colorCode;
            std::string resetCode;
            
            if (m_enableColors) {
                colorCode = GetColorCode(originalInfo.level);
                resetCode = GetResetColorCode();
            }
            
            // Convert wide string to multi-byte string
            std::string logStr = WStringToString(formattedLog);
            
            // Get log level prefix
            std::string levelPrefix = GetLogLevelPrefix(originalInfo.level);
            
            // Choose output stream
            std::ostream& outStream = ShouldUseStderr(originalInfo.level) ? std::cerr : std::cout;
            
            // Write with level prefix, color codes if enabled
            outStream << colorCode << levelPrefix << " " << logStr << resetCode << std::endl;
            
            return LogOutputResult::Success;
        }
    }
    catch (const std::exception&) {
        return LogOutputResult::Failed;
    }
}

void ConsoleLogOutput::FlushInternal() {
    std::cout.flush();
    std::cerr.flush();
}

bool ConsoleLogOutput::IsAvailableInternal() const {
    return true; // Console is always available
}

bool ConsoleLogOutput::InitializeInternal(const std::wstring& config) {
#ifdef _WIN32
    // Enable UTF-8 output on Windows console
    SetConsoleOutputCP(CP_UTF8);
    
    // Enable ANSI color support on Windows 10+
    if (m_enableColors) {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
        DWORD dwMode = 0;
        
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
        
        if (GetConsoleMode(hErr, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hErr, dwMode);
        }
    }
#endif
    
    return true;
}

void ConsoleLogOutput::ShutdownInternal() {
    FlushInternal();
}

std::wstring ConsoleLogOutput::GetConfigStringInternal() const {
    std::wstring config = L"Console:{";
    config += L"useStderr:" + std::wstring(m_useStderr ? L"true" : L"false");
    config += L",enableColors:" + std::wstring(m_enableColors ? L"true" : L"false");
    config += L",stderrThreshold:" + std::to_wstring(static_cast<int>(m_stderrThreshold));
    config += L"}";
    return config;
}

// Private helper methods
std::string ConsoleLogOutput::GetColorCode(LogLevel level) const {
    if (!m_enableColors) {
        return "";
    }
    
    // ANSI color codes
    static const std::map<LogLevel, std::string> colorMap = {
        {LogLevel::Trace,     "\033[37m"},      // White
        {LogLevel::Debug,     "\033[36m"},      // Cyan
        {LogLevel::Info,      "\033[32m"},      // Green
        {LogLevel::Notice,    "\033[34m"},      // Blue
        {LogLevel::Warning,   "\033[33m"},      // Yellow
        {LogLevel::Error,     "\033[31m"},      // Red
        {LogLevel::Critical,  "\033[35m"},      // Magenta
        {LogLevel::Alert,     "\033[1;31m"},    // Bright Red
        {LogLevel::Emergency, "\033[1;35m"},    // Bright Magenta
        {LogLevel::Fatal,     "\033[1;41m"}     // Bright Red Background
    };
    
    auto it = colorMap.find(level);
    return (it != colorMap.end()) ? it->second : "";
}

std::string ConsoleLogOutput::GetResetColorCode() const {
    return m_enableColors ? "\033[0m" : "";
}

bool ConsoleLogOutput::ShouldUseStderr(LogLevel level) const {
    return m_useStderr && (static_cast<int>(level) >= static_cast<int>(m_stderrThreshold));
}

std::string ConsoleLogOutput::WStringToString(const std::wstring& wstr) const {
    try {
        // Use UniConv to convert wide string to locale
        return UniConv::GetInstance()->WideStringToLocale(wstr);
    }
    catch (...) {
        // Fallback to simple conversion
        std::string result;
        result.reserve(wstr.length());
        for (wchar_t wc : wstr) {
            if (wc <= 127) {
                result.push_back(static_cast<char>(wc));
            } else {
                result.push_back('?'); // Replace non-ASCII with '?'
            }
        }
        return result;
    }
}

std::string ConsoleLogOutput::GetLogLevelPrefix(LogLevel level) const {
    // 返回日志级别的字符串前缀
    static const std::map<LogLevel, std::string> levelPrefixMap = {
        {LogLevel::Trace,     "[TRACE]"},
        {LogLevel::Debug,     "[DEBUG]"},
        {LogLevel::Info,      "[INFO]"},
        {LogLevel::Notice,    "[NOTICE]"},
        {LogLevel::Warning,   "[WARNING]"},
        {LogLevel::Error,     "[ERROR]"},
        {LogLevel::Critical,  "[CRITICAL]"},
        {LogLevel::Alert,     "[ALERT]"},
        {LogLevel::Emergency, "[EMERGENCY]"},
        {LogLevel::Fatal,     "[FATAL]"}
    };
    
    auto it = levelPrefixMap.find(level);
    return (it != levelPrefixMap.end()) ? it->second : "[UNKNOWN]";
}

void ConsoleLogOutput::SetUseSeparateConsole(bool useSeparateConsole) {
    if (m_useSeparateConsole == useSeparateConsole) {
        return; // No change needed
    }
    
    if (m_useSeparateConsole && !useSeparateConsole) {
        // Shutdown separate console
        ShutdownSeparateConsole();
    }
    
    m_useSeparateConsole = useSeparateConsole;
    
    if (!m_useSeparateConsole && useSeparateConsole) {
        // Initialize separate console
        InitializeSeparateConsole();
    }
}

void ConsoleLogOutput::InitializeSeparateConsole() {
#ifdef _WIN32
    LIGHTLOG_DEBUG_CONSOLE_INFO(L"Initializing separate console process...");
    
    // 创建匿名管道用于与独立控制台进程通信
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;  // 管道句柄需要被子进程继承
    saAttr.lpSecurityDescriptor = NULL;
    
    // 创建管道：m_consolePipeRead 用于子进程读取，m_consolePipeWrite 用于父进程写入
    if (!CreatePipe(&m_consolePipeRead, &m_consolePipeWrite, &saAttr, 0)) {
        std::wcout << L"[ERROR] Failed to create pipe. Error: " << GetLastError() << std::endl;
        return;
    }
    
    // 确保写入句柄不被子进程继承
    if (!SetHandleInformation(m_consolePipeWrite, HANDLE_FLAG_INHERIT, 0)) {
        std::wcout << L"[ERROR] Failed to set handle information. Error: " << GetLastError() << std::endl;
        CloseHandle(m_consolePipeRead);
        CloseHandle(m_consolePipeWrite);
        return;
    }
    
    // 准备启动独立的控制台进程
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdInput = m_consolePipeRead;   // 子进程从管道读取
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);  // 使用新控制台的输出
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);    // 使用新控制台的错误输出
    si.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pi, sizeof(pi));
    
    // 创建命令行：启动一个简单的控制台来显示日志
    // 使用 findstr 命令来持续读取管道数据并显示（findstr . 会显示所有非空行）
    wchar_t cmdLine[] = L"cmd.exe /k \"title LightLog - Separate Console Output && echo === LightLog Separate Console === && echo Waiting for log messages... && findstr .\"";
    
    // 创建新的控制台进程
    if (CreateProcessW(NULL,               // 不指定程序名
                      cmdLine,                // 命令行
                      NULL,              // 进程安全属性
                      NULL,               // 线程安全属性
                      TRUE,                 // 继承句柄
                      CREATE_NEW_CONSOLE,   // 创建新控制台窗口
                      NULL,                  // 环境变量
                      NULL,              // 当前目录
                      &si,                   // 启动信息
                      &pi))             // 进程信息
    {
        LIGHTLOG_DEBUG_CONSOLE_INFO(L"Separate console process created successfully!");
        LIGHTLOG_DEBUG_STREAM(DEBUG_LEVEL_INFO, Console) << L"Process ID: " << pi.dwProcessId LIGHTLOG_DEBUG_STREAM_END(INFO, Console);
        
        // 保存进程句柄
        m_consoleProcess = pi.hProcess;
        CloseHandle(pi.hThread);  // 不需要线程句柄
        
        // 关闭子进程的读取端（父进程不需要）
        CloseHandle(m_consolePipeRead);
        m_consolePipeRead = INVALID_HANDLE_VALUE;
        
        // 启动控制台处理线程
        m_consoleThreadRunning = true;
        m_shutdownRequested = false;
        m_consoleThread = std::thread(&ConsoleLogOutput::ConsoleThreadProc, this);
        
        LIGHTLOG_DEBUG_CONSOLE_INFO(L"Console thread started successfully!");
    }
    else {
        DWORD error = GetLastError();
        std::wcout << L"[ERROR] Failed to create console process. Error: " << error << std::endl;
        CloseHandle(m_consolePipeRead);
        CloseHandle(m_consolePipeWrite);
        m_consolePipeRead = INVALID_HANDLE_VALUE;
        m_consolePipeWrite = INVALID_HANDLE_VALUE;
    }
#endif
}

void ConsoleLogOutput::ShutdownSeparateConsole() {
    if (m_consoleThreadRunning.load()) {
        // Signal shutdown
        m_shutdownRequested = true;
        
        // Notify the console thread
        m_queueCondition.notify_one();
        
        // Wait for thread to finish
        if (m_consoleThread.joinable()) {
            m_consoleThread.join();
        }
        
        m_consoleThreadRunning = false;
        
#ifdef _WIN32
        // Close pipe handles
        if (m_consolePipeWrite != INVALID_HANDLE_VALUE) {
            CloseHandle(m_consolePipeWrite);
            m_consolePipeWrite = INVALID_HANDLE_VALUE;
        }
        
        if (m_consolePipeRead != INVALID_HANDLE_VALUE) {
            CloseHandle(m_consolePipeRead);
            m_consolePipeRead = INVALID_HANDLE_VALUE;
        }
        
        // Terminate the console process
        if (m_consoleProcess != INVALID_HANDLE_VALUE) {
            LIGHTLOG_DEBUG_CONSOLE_INFO(L"Terminating separate console process...");
            TerminateProcess(m_consoleProcess, 0);
            CloseHandle(m_consoleProcess);
            m_consoleProcess = INVALID_HANDLE_VALUE;
        }
#endif
    }
}

void ConsoleLogOutput::ConsoleThreadProc() {
    LIGHTLOG_DEBUG_CONSOLE_INFO(L"Console thread started running!");
    
    while (!m_shutdownRequested.load()) {
        ConsoleLogItem item;
        
        if (DequeueLogItem(item)) {
            LIGHTLOG_DEBUG_CONSOLE_VERBOSE(L"Processing log item...");
            
            // Process the log item and send to separate console via pipe
            try {
#ifdef _WIN32
                if (m_consolePipeWrite != INVALID_HANDLE_VALUE) {
                    std::string colorCode;
                    std::string resetCode;
                    
                    if (m_enableColors) {
                        colorCode = GetColorCode(item.level);
                        resetCode = GetResetColorCode();
                    }
                    
                    // Convert wide string to multi-byte string
                    std::string logStr = WStringToString(item.formattedLog);
                    
                    // Get log level prefix
                    std::string levelPrefix = GetLogLevelPrefix(item.level);
                    
                    // Format the complete message with level prefix and colors
                    std::string fullMessage = colorCode + levelPrefix + " " + logStr + resetCode + "\n";
                    
                    // Write to pipe (which will be displayed in the separate console)
                    DWORD bytesWritten;
                    if (WriteFile(m_consolePipeWrite, fullMessage.c_str(), fullMessage.length(), &bytesWritten, NULL)) {
                        // 刷新管道确保立即显示
                        FlushFileBuffers(m_consolePipeWrite);
                        LIGHTLOG_DEBUG_STREAM(DEBUG_LEVEL_VERBOSE, Console) << L"Sent log message to separate console: " << bytesWritten << L" bytes" LIGHTLOG_DEBUG_STREAM_END(VERBOSE, Console);
                    } else {
                        std::wcout << L"[ERROR] Failed to write to pipe. Error: " << GetLastError() << std::endl;
                    }
                } else {
                    std::wcout << L"[ERROR] Pipe handle is invalid!" << std::endl;
                }
                // Fallback for non-Windows: use standard console output
                std::string colorCode;
                std::string resetCode;
                
                if (m_enableColors) {
                    colorCode = GetColorCode(item.level);
                    resetCode = GetResetColorCode();
                }
                
                std::string logStr = WStringToString(item.formattedLog);
                std::ostream& outStream = ShouldUseStderr(item.level) ? std::cerr : std::cout;
                outStream << colorCode << logStr << resetCode << std::endl;
                outStream.flush();
#endif
            }
            catch (...) {
                // Ignore errors in console thread
            }
        }
        else {
            // Wait for new items or shutdown signal
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCondition.wait_for(lock, std::chrono::milliseconds(100));
        }
    }
}

void ConsoleLogOutput::EnqueueLogItem(const ConsoleLogItem& item) {
    // 调试信息：日志项入队
    LIGHTLOG_DEBUG_CONSOLE_VERBOSE(L"Enqueuing log item to separate console...");
    
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_logQueue.push(item);
    }
    m_queueCondition.notify_one();
}

bool ConsoleLogOutput::DequeueLogItem(ConsoleLogItem& item) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if (!m_logQueue.empty()) {
        item = m_logQueue.front();
        m_logQueue.pop();
        return true;
    }
    return false;
}