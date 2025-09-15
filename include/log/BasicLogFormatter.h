#pragma once

#include "ILogFormatter.h"
#include <regex>
#include <mutex>

/**
 * @brief Basic log formatter implementation
 * @details Provides standard log formatting with pattern support
 */
class BasicLogFormatter : public ILogFormatter {
private:
	LogFormatConfig m_config;
	mutable std::mutex m_configMutex;

	// Cached patterns for performance
	std::wregex m_tokenRegex;
	bool m_regexCached;

public:
	/**
	 * @brief Constructor
	 * @param config Initial format configuration
	 */
	explicit BasicLogFormatter(const LogFormatConfig& config = LogFormatConfig{});

	/**
	 * @brief Destructor
	 */
	virtual ~BasicLogFormatter();

	// ILogFormatter interface implementation
	std::wstring FormatLog(const LogCallbackInfo& logInfo) override;
	void SetConfig(const LogFormatConfig& config) override;
	LogFormatConfig GetConfig() const override;
	std::wstring GetFormatterType() const override;

private:
	// Helper methods
	void BuildTokenRegex();
	std::wstring FormatTimestamp(const std::chrono::system_clock::time_point& timestamp, const std::wstring& format) const;
	std::wstring LogLevelToString(LogLevel level) const;
	std::wstring GetColorCode(LogLevel level) const;
	std::wstring GetResetColor() const;
	std::wstring ProcessPattern(const std::wstring& pattern, const LogCallbackInfo& logInfo) const;
};