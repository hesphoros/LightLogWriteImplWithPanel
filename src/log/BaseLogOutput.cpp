#include "../../include/log/BaseLogOutput.h"
#include <chrono>

BaseLogOutput::BaseLogOutput(const std::wstring& outputName, const std::wstring& outputType)
    : m_outputName(outputName)
    , m_outputType(outputType)
    , m_minLogLevel(LogLevel::Trace)
    , m_enabled(true)
    , m_initialized(false)
    , m_formatter(nullptr)
    , m_filter(nullptr)
{
    // Initialize statistics
    m_stats.totalLogs = 0;
    m_stats.successfulLogs = 0;
    m_stats.failedLogs = 0;
    m_stats.filteredLogs = 0;
    m_stats.bytesWritten = 0;
    m_stats.averageWriteTime = 0.0;
    m_stats.lastWriteTime = std::chrono::system_clock::now();
}

BaseLogOutput::~BaseLogOutput() {
    if (m_initialized.load()) {
        Shutdown();
    }
}

LogOutputResult BaseLogOutput::WriteLog(const LogCallbackInfo& logInfo) {
    std::wcout << L"[DEBUG] BaseLogOutput::WriteLog called for output: " << this->GetOutputName() << std::endl;
    std::wcout << L"[DEBUG] enabled=" << (m_enabled.load() ? L"true" : L"false") 
               << L", initialized=" << (m_initialized.load() ? L"true" : L"false") << std::endl;
    
    // Check if output is enabled and initialized
    if (!m_enabled.load() || !m_initialized.load()) {
        std::wcout << L"[DEBUG] Output not available - returning Unavailable" << std::endl;
        return LogOutputResult::Unavailable;
    }

    // Check log level
    if (!ShouldLogLevel(logInfo.level)) {
        return LogOutputResult::Filtered;
    }

    // Apply filter if present
    LogCallbackInfo transformedInfo = logInfo;
    if (m_filter) {
        FilterOperation filterResult = ApplyFilter(logInfo, &transformedInfo);
        switch (filterResult) {
            case FilterOperation::Block:
                return LogOutputResult::Filtered;
            case FilterOperation::Allow:
                // Use original info
                break;
            case FilterOperation::Transform:
                // Use transformed info
                break;
        }
    }

    // Format the log message
    std::wstring formattedLog = FormatLogMessage(transformedInfo);
    if (formattedLog.empty()) {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_stats.failedLogs++;
        return LogOutputResult::Failed;
    }

    // Measure write time
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Write the log
    LogOutputResult result;
    {
        std::lock_guard<std::mutex> lock(m_outputMutex);
        result = WriteLogInternal(formattedLog, transformedInfo);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    double writeTime = duration.count() / 1000.0; // Convert to milliseconds

    // Update statistics
    UpdateStats(result, writeTime, formattedLog.length() * sizeof(wchar_t));

    return result;
}

void BaseLogOutput::Flush() {
    if (!m_initialized.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_outputMutex);
    FlushInternal();
}

bool BaseLogOutput::IsAvailable() const {
    return m_initialized.load() && IsAvailableInternal();
}

bool BaseLogOutput::Initialize(const std::wstring& config) {
    if (m_initialized.load()) {
        return true; // Already initialized
    }

    if (InitializeInternal(config)) {
        m_initialized = true;
        return true;
    }
    
    return false;
}

void BaseLogOutput::Shutdown() {
    if (!m_initialized.load()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_outputMutex);
        ShutdownInternal();
    }
    
    m_initialized = false;
}

std::wstring BaseLogOutput::GetConfigString() const {
    return GetConfigStringInternal();
}

std::wstring BaseLogOutput::GetOutputName() const {
    return m_outputName;
}

std::wstring BaseLogOutput::GetOutputType() const {
    return m_outputType;
}

void BaseLogOutput::SetFormatter(std::shared_ptr<ILogFormatter> formatter) {
    std::lock_guard<std::mutex> lock(m_outputMutex);
    m_formatter = formatter;
}

std::shared_ptr<ILogFormatter> BaseLogOutput::GetFormatter() const {
    std::lock_guard<std::mutex> lock(m_outputMutex);
    return m_formatter;
}

void BaseLogOutput::SetFilter(std::shared_ptr<ILogFilter> filter) {
    std::lock_guard<std::mutex> lock(m_outputMutex);
    m_filter = filter;
}

std::shared_ptr<ILogFilter> BaseLogOutput::GetFilter() const {
    std::lock_guard<std::mutex> lock(m_outputMutex);
    return m_filter;
}

LogOutputStats BaseLogOutput::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
}

void BaseLogOutput::ResetStatistics() {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_stats.totalLogs = 0;
    m_stats.successfulLogs = 0;
    m_stats.failedLogs = 0;
    m_stats.filteredLogs = 0;
    m_stats.bytesWritten = 0;
    m_stats.averageWriteTime = 0.0;
    m_stats.lastWriteTime = std::chrono::system_clock::now();
}

void BaseLogOutput::SetMinLogLevel(LogLevel minLevel) {
    m_minLogLevel = minLevel;
}

LogLevel BaseLogOutput::GetMinLogLevel() const {
    return m_minLogLevel;
}

void BaseLogOutput::SetEnabled(bool enabled) {
    m_enabled = enabled;
}

bool BaseLogOutput::IsEnabled() const {
    return m_enabled.load();
}

// Protected helper methods
void BaseLogOutput::UpdateStats(LogOutputResult result, double writeTime, size_t bytesWritten) {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    
    m_stats.totalLogs++;
    
    if (result == LogOutputResult::Success) {
        m_stats.successfulLogs++;
        m_stats.bytesWritten += bytesWritten;
    } else {
        m_stats.failedLogs++;
    }
    
    m_stats.lastWriteTime = std::chrono::system_clock::now();
    
    // Update rolling average
    if (m_stats.totalLogs == 1) {
        m_stats.averageWriteTime = writeTime;
    } else {
        m_stats.averageWriteTime = (m_stats.averageWriteTime * (m_stats.totalLogs - 1) + writeTime) / m_stats.totalLogs;
    }
}

bool BaseLogOutput::ShouldLogLevel(LogLevel level) const {
    return static_cast<int>(level) >= static_cast<int>(m_minLogLevel);
}

std::wstring BaseLogOutput::FormatLogMessage(const LogCallbackInfo& logInfo) {
    if (m_formatter) {
        return m_formatter->FormatLog(logInfo);
    }
    
    // Default simple formatting
    return logInfo.message;
}

FilterOperation BaseLogOutput::ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo) {
    if (!m_filter || !m_filter->IsEnabled()) {
        return FilterOperation::Allow;
    }
    
    return m_filter->ApplyFilter(logInfo, transformedInfo);
}