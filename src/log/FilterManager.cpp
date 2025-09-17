#include "log/FilterManager.h"
#include <sstream>
#include <algorithm>

FilterManager::FilterManager()
    : m_globalFilterEnabled(true)
    , m_defaultPriority(static_cast<int>(FilterPriority::Normal))
{
    RegisterBuiltinFilterTypes();
}

void FilterManager::RegisterFilterType(const std::wstring& typeName, FilterFactory factory) {
    std::lock_guard<std::mutex> lock(m_factoriesMutex);
    m_filterFactories[typeName] = factory;
}

void FilterManager::UnregisterFilterType(const std::wstring& typeName) {
    std::lock_guard<std::mutex> lock(m_factoriesMutex);
    m_filterFactories.erase(typeName);
}

std::unique_ptr<ILogFilter> FilterManager::CreateFilter(const std::wstring& typeName) {
    std::lock_guard<std::mutex> lock(m_factoriesMutex);
    
    auto it = m_filterFactories.find(typeName);
    if (it != m_filterFactories.end()) {
        auto filter = it->second();
        if (filter) {
            filter->SetPriority(m_defaultPriority);
        }
        return filter;
    }
    
    return nullptr;
}

std::unique_ptr<ILogFilter> FilterManager::CreateFilter(const std::wstring& typeName, const std::wstring& config) {
    auto filter = CreateFilter(typeName);
    if (filter && !config.empty()) {
        filter->SetConfiguration(config);
    }
    return filter;
}

std::vector<std::wstring> FilterManager::GetAvailableFilterTypes() const {
    std::lock_guard<std::mutex> lock(m_factoriesMutex);
    
    std::vector<std::wstring> types;
    for (const auto& pair : m_filterFactories) {
        types.push_back(pair.first);
    }
    
    return types;
}

bool FilterManager::IsFilterTypeRegistered(const std::wstring& typeName) const {
    std::lock_guard<std::mutex> lock(m_factoriesMutex);
    return m_filterFactories.find(typeName) != m_filterFactories.end();
}

void FilterManager::SaveFilterConfiguration(const std::wstring& name, const std::shared_ptr<ILogFilter>& filter) {
    if (!filter) return;
    
    std::lock_guard<std::mutex> lock(m_configurationsMutex);
    m_savedConfigurations[name] = filter->Clone();
}

std::shared_ptr<ILogFilter> FilterManager::LoadFilterConfiguration(const std::wstring& name) {
    std::lock_guard<std::mutex> lock(m_configurationsMutex);
    
    auto it = m_savedConfigurations.find(name);
    if (it != m_savedConfigurations.end()) {
        return std::shared_ptr<ILogFilter>(it->second->Clone().release());
    }
    
    return nullptr;
}

void FilterManager::DeleteFilterConfiguration(const std::wstring& name) {
    std::lock_guard<std::mutex> lock(m_configurationsMutex);
    m_savedConfigurations.erase(name);
}

std::vector<std::wstring> FilterManager::GetSavedConfigurations() const {
    std::lock_guard<std::mutex> lock(m_configurationsMutex);
    
    std::vector<std::wstring> names;
    for (const auto& pair : m_savedConfigurations) {
        names.push_back(pair.first);
    }
    
    return names;
}

bool FilterManager::ConfigurationExists(const std::wstring& name) const {
    std::lock_guard<std::mutex> lock(m_configurationsMutex);
    return m_savedConfigurations.find(name) != m_savedConfigurations.end();
}

FilterValidationResult FilterManager::ValidateFilter(const std::shared_ptr<ILogFilter>& filter) {
    FilterValidationResult result;
    result.isValid = true;
    
    if (!filter) {
        result.isValid = false;
        result.errors.push_back(L"Filter is null");
        return result;
    }
    
    // Basic validation
    if (filter->GetFilterName().empty()) {
        result.warnings.push_back(L"Filter name is empty");
    }
    
    // Validate configuration
    std::wstring config = filter->GetConfiguration();
    if (!filter->ValidateConfiguration(config)) {
        result.isValid = false;
        result.errors.push_back(L"Invalid filter configuration");
    }
    
    return result;
}

FilterValidationResult FilterManager::ValidateConfiguration(const std::wstring& filterType, const std::wstring& configuration) {
    FilterValidationResult result;
    result.isValid = true;
    
    auto filter = CreateFilter(filterType);
    if (!filter) {
        result.isValid = false;
        result.errors.push_back(L"Unknown filter type: " + filterType);
        return result;
    }
    
    if (!filter->ValidateConfiguration(configuration)) {
        result.isValid = false;
        result.errors.push_back(L"Invalid configuration for filter type: " + filterType);
    }
    
    return result;
}

void FilterManager::CreateFilterTemplate(const std::wstring& templateName, const std::wstring& filterType, const std::wstring& defaultConfig) {
    std::lock_guard<std::mutex> lock(m_templatesMutex);
    m_templates[templateName] = std::make_pair(filterType, defaultConfig);
}

std::unique_ptr<ILogFilter> FilterManager::CreateFromTemplate(const std::wstring& templateName) {
    std::lock_guard<std::mutex> lock(m_templatesMutex);
    
    auto it = m_templates.find(templateName);
    if (it != m_templates.end()) {
        return CreateFilter(it->second.first, it->second.second);
    }
    
    return nullptr;
}

std::vector<std::wstring> FilterManager::GetAvailableTemplates() const {
    std::lock_guard<std::mutex> lock(m_templatesMutex);
    
    std::vector<std::wstring> templates;
    for (const auto& pair : m_templates) {
        templates.push_back(pair.first);
    }
    
    return templates;
}

std::unique_ptr<ICompositeFilter> FilterManager::CreateCompositeFilter(CompositionStrategy strategy) {
    return std::make_unique<CompositeFilter>(L"CompositeFilter", strategy);
}

std::unique_ptr<ICompositeFilter> FilterManager::CreateCompositeFilterFromConfig(const std::wstring& config) {
    // Simple implementation - parse basic strategy config
    CompositionStrategy strategy = CompositionStrategy::AllMustPass;
    
    if (config.find(L"AnyCanPass") != std::wstring::npos) {
        strategy = CompositionStrategy::AnyCanPass;
    }
    else if (config.find(L"MajorityRule") != std::wstring::npos) {
        strategy = CompositionStrategy::MajorityRule;
    }
    else if (config.find(L"FirstMatch") != std::wstring::npos) {
        strategy = CompositionStrategy::FirstMatch;
    }
    
    return CreateCompositeFilter(strategy);
}

void FilterManager::SetGlobalFilterEnabled(bool enabled) {
    m_globalFilterEnabled = enabled;
}

bool FilterManager::IsGlobalFilterEnabled() const {
    return m_globalFilterEnabled;
}

void FilterManager::SetDefaultPriority(int priority) {
    m_defaultPriority = priority;
}

int FilterManager::GetDefaultPriority() const {
    return m_defaultPriority;
}

std::map<std::wstring, FilterStatistics> FilterManager::GetAllFilterStatistics() const {
    std::lock_guard<std::mutex> lock(m_configurationsMutex);
    
    std::map<std::wstring, FilterStatistics> allStats;
    
    for (const auto& pair : m_savedConfigurations) {
        if (pair.second) {
            allStats[pair.first] = pair.second->GetStatistics();
        }
    }
    
    return allStats;
}

void FilterManager::ResetAllStatistics() {
    std::lock_guard<std::mutex> lock(m_configurationsMutex);
    
    for (const auto& pair : m_savedConfigurations) {
        if (pair.second) {
            pair.second->ResetStatistics();
        }
    }
}

std::wstring FilterManager::ExportConfiguration(const std::wstring& configName) const {
    std::lock_guard<std::mutex> lock(m_configurationsMutex);
    
    auto it = m_savedConfigurations.find(configName);
    if (it != m_savedConfigurations.end()) {
        return SerializeFilter(it->second);
    }
    
    return L"";
}

bool FilterManager::ImportConfiguration(const std::wstring& configName, const std::wstring& configData) {
    auto filter = DeserializeFilter(configData);
    if (filter) {
        std::lock_guard<std::mutex> lock(m_configurationsMutex);
        m_savedConfigurations[configName] = filter;
        return true;
    }
    
    return false;
}

void FilterManager::RegisterBuiltinFilterTypes() {
    // Register built-in filter types
    RegisterFilterType(L"LevelFilter", []() {
        return std::make_unique<LevelFilter>();
    });
    
    RegisterFilterType(L"KeywordFilter", []() {
        return std::make_unique<KeywordFilter>();
    });
    
    RegisterFilterType(L"RegexFilter", []() {
        return std::make_unique<RegexFilter>();
    });
    
    RegisterFilterType(L"RateLimitFilter", []() {
        return std::make_unique<RateLimitFilter>();
    });
    
    RegisterFilterType(L"ThreadFilter", []() {
        return std::make_unique<ThreadFilter>();
    });
    
    RegisterFilterType(L"CompositeFilter", []() -> std::unique_ptr<ILogFilter> {
        return std::make_unique<CompositeFilter>();
    });
    
    // Create common templates
    CreateFilterTemplate(L"ErrorOnly", L"LevelFilter", L"minLevel=Error");
    CreateFilterTemplate(L"WarningAndAbove", L"LevelFilter", L"minLevel=Warning");
    CreateFilterTemplate(L"DebugFilter", L"LevelFilter", L"minLevel=Debug,maxLevel=Debug");
    CreateFilterTemplate(L"SlowRate", L"RateLimitFilter", L"maxPerSecond=10,maxBurst=5");
    CreateFilterTemplate(L"FastRate", L"RateLimitFilter", L"maxPerSecond=1000,maxBurst=100");
}

std::wstring FilterManager::SerializeFilter(const std::shared_ptr<ILogFilter>& filter) const {
    if (!filter) return L"";
    
    std::wostringstream oss;
    oss << L"FilterType=" << filter->GetFilterName() << L";"
        << L"Enabled=" << (filter->IsEnabled() ? L"true" : L"false") << L";"
        << L"Priority=" << filter->GetPriority() << L";"
        << L"Configuration=" << filter->GetConfiguration() << L";";
    
    return oss.str();
}

std::shared_ptr<ILogFilter> FilterManager::DeserializeFilter(const std::wstring& data) const {
    // Simple deserialization - parse key=value pairs
    std::map<std::wstring, std::wstring> props;
    
    std::wistringstream iss(data);
    std::wstring token;
    
    while (std::getline(iss, token, L';')) {
        size_t pos = token.find(L'=');
        if (pos != std::wstring::npos) {
            std::wstring key = token.substr(0, pos);
            std::wstring value = token.substr(pos + 1);
            props[key] = value;
        }
    }
    
    auto typeIt = props.find(L"FilterType");
    if (typeIt == props.end()) return nullptr;
    
    std::lock_guard<std::mutex> lock(m_factoriesMutex);
    auto factoryIt = m_filterFactories.find(typeIt->second);
    if (factoryIt == m_filterFactories.end()) return nullptr;
    
    auto filter = factoryIt->second();
    if (!filter) return nullptr;
    
    auto enabledIt = props.find(L"Enabled");
    if (enabledIt != props.end()) {
        filter->SetEnabled(enabledIt->second == L"true");
    }
    
    auto priorityIt = props.find(L"Priority");
    if (priorityIt != props.end()) {
        filter->SetPriority(std::stoi(priorityIt->second));
    }
    
    auto configIt = props.find(L"Configuration");
    if (configIt != props.end()) {
        filter->SetConfiguration(configIt->second);
    }
    
    return std::shared_ptr<ILogFilter>(filter.release());
}