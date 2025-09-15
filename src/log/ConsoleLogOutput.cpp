#include "../../include/log/ConsoleLogOutput.h"
#include "../../include/log/UniConv.h"
#include <iostream>
#include <map>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#endif

ConsoleLogOutput::ConsoleLogOutput(const std::wstring& outputName, bool useStderr, bool enableColors)
    : BaseLogOutput(outputName, L"Console")
    , m_useStderr(useStderr)
    , m_enableColors(enableColors)
    , m_stderrThreshold(LogLevel::Warning)
{
}

ConsoleLogOutput::~ConsoleLogOutput() {
}

LogOutputResult ConsoleLogOutput::WriteLogInternal(const std::wstring& formattedLog, const LogCallbackInfo& originalInfo) {
    try {
        std::string colorCode;
        std::string resetCode;
        
        if (m_enableColors) {
            colorCode = GetColorCode(originalInfo.level);
            resetCode = GetResetColorCode();
        }
        
        // Convert wide string to multi-byte string
        std::string logStr = WStringToString(formattedLog);
        
        // Choose output stream
        std::ostream& outStream = ShouldUseStderr(originalInfo.level) ? std::cerr : std::cout;
        
        // Write with color codes if enabled
        outStream << colorCode << logStr << resetCode << std::endl;
        
        return LogOutputResult::Success;
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