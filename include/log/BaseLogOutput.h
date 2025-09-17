#pragma once

#include "ILogOutput.h"
#include "ILogFormatter.h"
#include "ILogFilter.h"
#include "LightLogWriteImpl.h"
#include <mutex>
#include <atomic>

/**
 * @brief Base implementation class for log outputs
 * @details Provides common functionality that most output implementations need,
 *          such as formatter/filter management, statistics tracking, and threading support.
 */
class BaseLogOutput : public ILogOutput {
protected:
    // Basic properties
    std::wstring m_outputName;
    std::wstring m_outputType;
    LogLevel m_minLogLevel;
    std::atomic<bool> m_enabled;
    std::atomic<bool> m_initialized;

    // Components
    LogFormatterPtr m_formatter;
    LogFilterPtr m_filter;

    // Statistics
    mutable std::mutex m_statsMutex;
    LogOutputStats m_stats;

    // Thread safety
    mutable std::mutex m_outputMutex;

public:
    /**
     * @brief Constructor
     * @param outputName Name of this output
     * @param outputType Type of this output
     */
    BaseLogOutput(const std::wstring& outputName, const std::wstring& outputType);

    /**
     * @brief Destructor
     */
    virtual ~BaseLogOutput();

    // ILogOutput interface implementation
    LogOutputResult WriteLog(const LogCallbackInfo& logInfo) override;
    void Flush() override;
    bool IsAvailable() const override;
    bool Initialize(const std::wstring& config = L"") override;
    void Shutdown() override;
    std::wstring GetConfigString() const override;

    // ILogOutput interface implementation
    std::wstring GetOutputName() const override;
    std::wstring GetOutputType() const override;
    void SetFormatter(std::shared_ptr<ILogFormatter> formatter) override;
    std::shared_ptr<ILogFormatter> GetFormatter() const override;
    void SetFilter(std::shared_ptr<ILogFilter> filter) override;
    std::shared_ptr<ILogFilter> GetFilter() const override;
    LogOutputStats GetStatistics() const override;
    void ResetStatistics() override;
    void SetMinLogLevel(LogLevel minLevel) override;
    LogLevel GetMinLogLevel() const override;
    void SetEnabled(bool enabled) override;
    bool IsEnabled() const override;

protected:
    // Virtual methods that derived classes must implement
    virtual LogOutputResult WriteLogInternal(const std::wstring& formattedLog, const LogCallbackInfo& originalInfo) = 0;
    virtual void FlushInternal() = 0;
    virtual bool IsAvailableInternal() const = 0;
    virtual bool InitializeInternal(const std::wstring& config) = 0;
    virtual void ShutdownInternal() = 0;
    virtual std::wstring GetConfigStringInternal() const = 0;

    // Helper methods for derived classes
    void UpdateStats(LogOutputResult result, double writeTime, size_t bytesWritten);
    bool ShouldLogLevel(LogLevel level) const;
    std::wstring FormatLogMessage(const LogCallbackInfo& logInfo);
    FilterOperation ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo = nullptr);
};