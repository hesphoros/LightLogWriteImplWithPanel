#pragma once

#include "BaseLogOutput.h"
#include <iostream>

/**
 * @brief Console output implementation
 * @details Writes logs to console (stdout/stderr) with optional color support
 */
class ConsoleLogOutput : public BaseLogOutput {
private:
    bool m_useStderr;           // Use stderr instead of stdout for error levels
    bool m_enableColors;        // Enable ANSI color codes
    LogLevel m_stderrThreshold; // Minimum level to use stderr

public:
    /**
     * @brief Constructor
     * @param outputName Name of this console output
     * @param useStderr Whether to use stderr for error levels
     * @param enableColors Whether to enable color output
     */
    ConsoleLogOutput(const std::wstring& outputName = L"Console", 
                     bool useStderr = true, 
                     bool enableColors = true);

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
};