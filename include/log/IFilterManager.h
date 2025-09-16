#pragma once

#include "ILogFilter.h"
#include "ICompositeFilter.h"
#include <functional>
#include <map>

/**
 * @brief Filter factory function type
 */
using FilterFactory = std::function<std::unique_ptr<ILogFilter>()>;

/**
 * @brief Filter configuration validation result
 */
struct FilterValidationResult {
	bool isValid;
	std::vector<std::wstring> errors;
	std::vector<std::wstring> warnings;
	std::map<std::wstring, std::wstring> suggestions;
};

/**
 * @brief Filter manager interface for centralized filter management
 * @details Provides factory pattern, configuration management, and filter registry
 */
class IFilterManager {
public:
	virtual ~IFilterManager() = default;
	
	// Filter registration and factory
	virtual void RegisterFilterType(const std::wstring& typeName, FilterFactory factory) = 0;
	virtual void UnregisterFilterType(const std::wstring& typeName) = 0;
	virtual std::unique_ptr<ILogFilter> CreateFilter(const std::wstring& typeName) = 0;
	virtual std::unique_ptr<ILogFilter> CreateFilter(const std::wstring& typeName, const std::wstring& config) = 0;
	virtual std::vector<std::wstring> GetAvailableFilterTypes() const = 0;
	virtual bool IsFilterTypeRegistered(const std::wstring& typeName) const = 0;
	
	// Configuration management
	virtual void SaveFilterConfiguration(const std::wstring& name, const std::shared_ptr<ILogFilter>& filter) = 0;
	virtual std::shared_ptr<ILogFilter> LoadFilterConfiguration(const std::wstring& name) = 0;
	virtual void DeleteFilterConfiguration(const std::wstring& name) = 0;
	virtual std::vector<std::wstring> GetSavedConfigurations() const = 0;
	virtual bool ConfigurationExists(const std::wstring& name) const = 0;
	
	// Filter validation
	virtual FilterValidationResult ValidateFilter(const std::shared_ptr<ILogFilter>& filter) = 0;
	virtual FilterValidationResult ValidateConfiguration(const std::wstring& filterType, const std::wstring& configuration) = 0;
	
	// Template and preset management
	virtual void CreateFilterTemplate(const std::wstring& templateName, const std::wstring& filterType, const std::wstring& defaultConfig) = 0;
	virtual std::unique_ptr<ILogFilter> CreateFromTemplate(const std::wstring& templateName) = 0;
	virtual std::vector<std::wstring> GetAvailableTemplates() const = 0;
	
	// Composite filter helpers
	virtual std::unique_ptr<ICompositeFilter> CreateCompositeFilter(CompositionStrategy strategy) = 0;
	virtual std::unique_ptr<ICompositeFilter> CreateCompositeFilterFromConfig(const std::wstring& config) = 0;
	
	// Global settings
	virtual void SetGlobalFilterEnabled(bool enabled) = 0;
	virtual bool IsGlobalFilterEnabled() const = 0;
	virtual void SetDefaultPriority(int priority) = 0;
	virtual int GetDefaultPriority() const = 0;
	
	// Statistics and monitoring
	virtual std::map<std::wstring, FilterStatistics> GetAllFilterStatistics() const = 0;
	virtual void ResetAllStatistics() = 0;
	
	// Import/Export
	virtual std::wstring ExportConfiguration(const std::wstring& configName) const = 0;
	virtual bool ImportConfiguration(const std::wstring& configName, const std::wstring& configData) = 0;
};

using FilterManagerPtr = std::shared_ptr<IFilterManager>;