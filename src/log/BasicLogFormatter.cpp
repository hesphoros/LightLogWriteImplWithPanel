#include "../../include/log/BasicLogFormatter.h"
#include "../../include/log/LightLogWriteImpl.h"  // For LogLevel and LogCallbackInfo
#include <iomanip>
#include <sstream>
#include <ctime>
#include <map>

BasicLogFormatter::BasicLogFormatter(const LogFormatConfig& config)
    : m_config(config), m_regexCached(false) {
}

BasicLogFormatter::~BasicLogFormatter() {
}

std::wstring BasicLogFormatter::FormatLog(const LogCallbackInfo& logInfo) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    
    std::wstring result;
    if (m_config.enableColors) {
        result += GetColorCode(logInfo.level);
    }
    
    result += ProcessPattern(m_config.pattern, logInfo);
    
    if (m_config.enableColors) {
        result += GetResetColor();
    }
    
    return result;
}

void BasicLogFormatter::SetConfig(const LogFormatConfig& config) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config = config;
    m_regexCached = false;
}

LogFormatConfig BasicLogFormatter::GetConfig() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config;
}

std::wstring BasicLogFormatter::GetFormatterType() const {
    return L"BasicLogFormatter";
}

void BasicLogFormatter::BuildTokenRegex() {
    if (m_regexCached) return;
    
    try {
        m_tokenRegex = std::wregex(L"\\{(\\w+)\\}");
        m_regexCached = true;
    } catch (...) {
        m_regexCached = false;
    }
}

std::wstring BasicLogFormatter::FormatTimestamp(const std::chrono::system_clock::time_point& timestamp, const std::wstring& format) const {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm;
    
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif
    
    std::wstringstream ss;
    ss << std::put_time(&tm, format.c_str());
    return ss.str();
}

std::wstring BasicLogFormatter::LogLevelToString(LogLevel level) const {
    static const std::map<LogLevel, std::wstring> levelMap = {
        {LogLevel::Trace, L"TRACE"},
        {LogLevel::Debug, L"DEBUG"},
        {LogLevel::Info, L"INFO"},
        {LogLevel::Notice, L"NOTICE"},
        {LogLevel::Warning, L"WARNING"},
        {LogLevel::Error, L"ERROR"},
        {LogLevel::Critical, L"CRITICAL"},
        {LogLevel::Alert, L"ALERT"},
        {LogLevel::Emergency, L"EMERGENCY"},
        {LogLevel::Fatal, L"FATAL"}
    };
    
    auto it = levelMap.find(level);
    return (it != levelMap.end()) ? it->second : L"UNKNOWN";
}

std::wstring BasicLogFormatter::GetColorCode(LogLevel level) const {
    if (!m_config.enableColors) return L"";
    
    auto it = m_config.levelColors.find(level);
    if (it != m_config.levelColors.end()) {
        // Convert LogColor to ANSI escape sequence
        switch (it->second) {
            case LogColor::Red: return L"\033[31m";
            case LogColor::Green: return L"\033[32m";
            case LogColor::Yellow: return L"\033[33m";
            case LogColor::Blue: return L"\033[34m";
            case LogColor::Magenta: return L"\033[35m";
            case LogColor::Cyan: return L"\033[36m";
            case LogColor::White: return L"\033[37m";
            case LogColor::BrightRed: return L"\033[1;31m";
            case LogColor::BrightGreen: return L"\033[1;32m";
            case LogColor::BrightYellow: return L"\033[1;33m";
            case LogColor::BrightBlue: return L"\033[1;34m";
            case LogColor::BrightMagenta: return L"\033[1;35m";
            case LogColor::BrightCyan: return L"\033[1;36m";
            case LogColor::BrightWhite: return L"\033[1;37m";
            default: return L"";
        }
    }
    
    return L"";
}

std::wstring BasicLogFormatter::GetResetColor() const {
    return m_config.enableColors ? L"\033[0m" : L"";
}

std::wstring BasicLogFormatter::ProcessPattern(const std::wstring& pattern, const LogCallbackInfo& logInfo) const {
    std::wstring result = pattern;
    
    // Simple token replacement
    size_t pos = 0;
    while ((pos = result.find(L"{timestamp}", pos)) != std::wstring::npos) {
        result.replace(pos, 11, FormatTimestamp(logInfo.timestamp, m_config.timestampFormat));
        pos += m_config.timestampFormat.length();
    }
    
    pos = 0;
    while ((pos = result.find(L"{level}", pos)) != std::wstring::npos) {
        result.replace(pos, 7, LogLevelToString(logInfo.level));
        pos += LogLevelToString(logInfo.level).length();
    }
    
    pos = 0;
    while ((pos = result.find(L"{message}", pos)) != std::wstring::npos) {
        result.replace(pos, 9, logInfo.message);
        pos += logInfo.message.length();
    }
    
    if (m_config.enableThreadId) {
        pos = 0;
        while ((pos = result.find(L"{threadid}", pos)) != std::wstring::npos) {
            std::wstringstream ss;
            ss << logInfo.threadId;
            result.replace(pos, 10, ss.str());
            pos += ss.str().length();
        }
    }
    
    return result;
}
