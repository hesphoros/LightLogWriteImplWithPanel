#pragma once

#include "IFilterManager.h"
#include "LogFilters.h"
#include "CompositeFilter.h"
#include <map>
#include <fstream>

/**
 * @brief Concrete implementation of filter manager
 */
class FilterManager : public IFilterManager {
private:
    std::map<std::wstring, FilterFactory> m_filterFactories;
    std::map<std::wstring, std::shared_ptr<ILogFilter>> m_savedConfigurations;
    std::map<std::wstring, std::pair<std::wstring, std::wstring>> m_templates; // name -> (type, config)
    
    mutable std::mutex m_factoriesMutex;
    mutable std::mutex m_configurationsMutex;
    mutable std::mutex m_templatesMutex;
    
    bool m_globalFilterEnabled;
    int m_defaultPriority;
    
public:
    FilterManager();
    virtual ~FilterManager() = default;
    
    // Factory registration
    void RegisterFilterType(const std::wstring& typeName, FilterFactory factory) override;
    void UnregisterFilterType(const std::wstring& typeName) override;
    std::unique_ptr<ILogFilter> CreateFilter(const std::wstring& typeName) override;
    std::unique_ptr<ILogFilter> CreateFilter(const std::wstring& typeName, const std::wstring& config) override;
    std::vector<std::wstring> GetAvailableFilterTypes() const override;
    bool IsFilterTypeRegistered(const std::wstring& typeName) const override;
    
    // Configuration management
    void SaveFilterConfiguration(const std::wstring& name, const std::shared_ptr<ILogFilter>& filter) override;
    std::shared_ptr<ILogFilter> LoadFilterConfiguration(const std::wstring& name) override;
    void DeleteFilterConfiguration(const std::wstring& name) override;
    std::vector<std::wstring> GetSavedConfigurations() const override;
    bool ConfigurationExists(const std::wstring& name) const override;
    
    // Filter validation
    FilterValidationResult ValidateFilter(const std::shared_ptr<ILogFilter>& filter) override;
    FilterValidationResult ValidateConfiguration(const std::wstring& filterType, const std::wstring& configuration) override;
    
    // Template management
    void CreateFilterTemplate(const std::wstring& templateName, const std::wstring& filterType, const std::wstring& defaultConfig) override;
    std::unique_ptr<ILogFilter> CreateFromTemplate(const std::wstring& templateName) override;
    std::vector<std::wstring> GetAvailableTemplates() const override;
    
    // Composite filter helpers
    std::unique_ptr<ICompositeFilter> CreateCompositeFilter(CompositionStrategy strategy) override;
    std::unique_ptr<ICompositeFilter> CreateCompositeFilterFromConfig(const std::wstring& config) override;
    
    // Global settings
    void SetGlobalFilterEnabled(bool enabled) override;
    bool IsGlobalFilterEnabled() const override;
    void SetDefaultPriority(int priority) override;
    int GetDefaultPriority() const override;
    
    // Statistics and monitoring
    std::map<std::wstring, FilterStatistics> GetAllFilterStatistics() const override;
    void ResetAllStatistics() override;
    
    // Import/Export
    std::wstring ExportConfiguration(const std::wstring& configName) const override;
    bool ImportConfiguration(const std::wstring& configName, const std::wstring& configData) override;
    
private:
    void RegisterBuiltinFilterTypes();
    std::wstring SerializeFilter(const std::shared_ptr<ILogFilter>& filter) const;
    std::shared_ptr<ILogFilter> DeserializeFilter(const std::wstring& data) const;
};