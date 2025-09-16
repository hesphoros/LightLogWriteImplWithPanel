#pragma once

#include "LogCommon.h"
#include <memory>
#include <string>
#include <chrono>
#include <map>
#include <vector>

enum class FilterOperation {
	Allow,
	Block,
	Transform
};

/**
 * @brief Filter priority levels for ordering multiple filters
 */
enum class FilterPriority {
	Lowest = -100,
	Low = -50,
	Normal = 0,
	High = 50,
	Highest = 100
};

/**
 * @brief Statistics for filter performance tracking
 */
struct FilterStatistics {
	size_t totalProcessed = 0;
	size_t allowed = 0;
	size_t blocked = 0;
	size_t transformed = 0;
	std::chrono::milliseconds totalProcessingTime{ 0 };
	double averageProcessingTime = 0.0;
	std::chrono::system_clock::time_point lastResetTime;
};

/**
 * @brief Filter context for enhanced filtering capabilities
 */
struct FilterContext {
	std::wstring loggerName;
	std::wstring sessionId;
	std::chrono::system_clock::time_point startTime;
	std::map<std::wstring, std::wstring> properties;
};

/**
 * @brief Enhanced log filter interface with configuration and performance tracking
 */
class ILogFilter {
public:
	virtual ~ILogFilter() = default;
	
	// Core filtering functionality
	virtual FilterOperation ApplyFilter(const LogCallbackInfo& logInfo, LogCallbackInfo* transformedInfo = nullptr) = 0;
	virtual bool IsEnabled() const = 0;
	virtual void SetEnabled(bool enabled) = 0;
	virtual std::wstring GetFilterName() const = 0;
	
	// Configuration management
	virtual bool SetConfiguration(const std::wstring& config) = 0;
	virtual std::wstring GetConfiguration() const = 0;
	virtual bool ValidateConfiguration(const std::wstring& config) const = 0;
	
	// Priority control
	virtual int GetPriority() const = 0;
	virtual void SetPriority(int priority) = 0;
	
	// Performance statistics
	virtual FilterStatistics GetStatistics() const = 0;
	virtual void ResetStatistics() = 0;
	
	// Performance optimization hints
	virtual bool CanQuickReject(LogLevel level) const = 0;
	virtual bool IsExpensive() const = 0;
	
	// Context management
	virtual void SetContext(const FilterContext& context) = 0;
	virtual FilterContext GetContext() const = 0;
	
	// Lifecycle management
	virtual std::unique_ptr<ILogFilter> Clone() const = 0;
	virtual void Reset() = 0;
	
	// Description and metadata
	virtual std::wstring GetDescription() const = 0;
	virtual std::wstring GetVersion() const = 0;
};

using LogFilterPtr = std::shared_ptr<ILogFilter>;
