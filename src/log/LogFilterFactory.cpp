#include "../../include/log/LogFilterFactory.h"
#include "../../include/log/UniConv.h"
#include <regex>

// 静态成员初始化
std::map<std::wstring, FilterTypeInfo> LogFilterFactory::filterTypes_;
bool LogFilterFactory::initialized_ = false;

void LogFilterFactory::Initialize() {
    if (initialized_) {
        return;
    }
    
    RegisterBuiltinFilters();
    initialized_ = true;
}

bool LogFilterFactory::RegisterFilterType(const std::wstring& typeName, const FilterTypeInfo& info) {
    if (typeName.empty()) {
        return false;
    }
    
    filterTypes_[typeName] = info;
    return true;
}

std::unique_ptr<ILogFilter> LogFilterFactory::CreateFilter(const std::wstring& typeName) {
    if (!initialized_) {
        Initialize();
    }
    
    auto it = filterTypes_.find(typeName);
    if (it == filterTypes_.end() || !it->second.creator) {
        return nullptr;
    }
    
    return it->second.creator();
}

std::unique_ptr<ILogFilter> LogFilterFactory::CreateFilterFromConfig(const std::wstring& typeName, const nlohmann::json& config) {
    auto filter = CreateFilter(typeName);
    if (!filter) {
        return nullptr;
    }
    
    auto it = filterTypes_.find(typeName);
    if (it != filterTypes_.end() && it->second.deserializer) {
        try {
            it->second.deserializer(filter.get(), config);
        }
        catch (...) {
            return nullptr;
        }
    }
    
    return filter;
}

nlohmann::json LogFilterFactory::SerializeFilter(const ILogFilter* filter) {
    if (!filter) {
        return nlohmann::json::object();
    }
    
    nlohmann::json result;
    result["type"] = WStringToString(filter->GetFilterName());
    result["enabled"] = filter->IsEnabled();
    result["priority"] = filter->GetPriority();
    result["description"] = WStringToString(filter->GetDescription());
    result["version"] = WStringToString(filter->GetVersion());
    
    // 使用类型特定的序列化器
    std::wstring typeName = filter->GetFilterName();
    auto it = filterTypes_.find(typeName);
    if (it != filterTypes_.end() && it->second.serializer) {
        try {
            result["config"] = it->second.serializer(filter);
        }
        catch (...) {
            result["config"] = nlohmann::json::object();
        }
    } else {
        // 后备方案：使用通用配置
        result["config"] = WStringToString(filter->GetConfiguration());
    }
    
    return result;
}

std::unique_ptr<ILogFilter> LogFilterFactory::DeserializeFilter(const nlohmann::json& json) {
    if (!initialized_) {
        Initialize();
    }
    
    if (!json.contains("type")) {
        return nullptr;
    }
    
    std::wstring typeName = StringToWString(json["type"].get<std::string>());
    auto filter = CreateFilter(typeName);
    if (!filter) {
        return nullptr;
    }
    
    try {
        // 设置基本属性
        if (json.contains("enabled")) {
            filter->SetEnabled(json["enabled"].get<bool>());
        }
        if (json.contains("priority")) {
            filter->SetPriority(json["priority"].get<int>());
        }
        
        // 使用类型特定的反序列化器
        if (json.contains("config")) {
            auto it = filterTypes_.find(typeName);
            if (it != filterTypes_.end() && it->second.deserializer) {
                it->second.deserializer(filter.get(), json["config"]);
            } else {
                // 后备方案：使用通用配置
                if (json["config"].is_string()) {
                    filter->SetConfiguration(StringToWString(json["config"].get<std::string>()));
                }
            }
        }
    }
    catch (...) {
        return nullptr;
    }
    
    return filter;
}

std::vector<std::wstring> LogFilterFactory::GetRegisteredTypes() {
    if (!initialized_) {
        Initialize();
    }
    
    std::vector<std::wstring> types;
    for (const auto& pair : filterTypes_) {
        types.push_back(pair.first);
    }
    return types;
}

const FilterTypeInfo* LogFilterFactory::GetTypeInfo(const std::wstring& typeName) {
    if (!initialized_) {
        Initialize();
    }
    
    auto it = filterTypes_.find(typeName);
    return it != filterTypes_.end() ? &it->second : nullptr;
}

bool LogFilterFactory::IsTypeRegistered(const std::wstring& typeName) {
    if (!initialized_) {
        Initialize();
    }
    
    return filterTypes_.find(typeName) != filterTypes_.end();
}

void LogFilterFactory::RegisterBuiltinFilters() {
    RegisterFilterType(L"Level", CreateLevelFilterInfo());
    RegisterFilterType(L"Keyword", CreateKeywordFilterInfo());
    RegisterFilterType(L"Regex", CreateRegexFilterInfo());
    RegisterFilterType(L"RateLimit", CreateFrequencyFilterInfo());
    RegisterFilterType(L"Thread", CreateThreadFilterInfo());
}

FilterTypeInfo LogFilterFactory::CreateLevelFilterInfo() {
    FilterTypeInfo info;
    info.typeName = L"Level";
    info.description = L"Filter logs by level range";
    info.isBuiltin = true;
    
    info.creator = []() -> std::unique_ptr<ILogFilter> {
        return std::make_unique<LevelFilter>();
    };
    
    info.serializer = [](const ILogFilter* filter) -> nlohmann::json {
        const LevelFilter* levelFilter = dynamic_cast<const LevelFilter*>(filter);
        if (!levelFilter) {
            return nlohmann::json::object();
        }
        
        nlohmann::json config;
        config["minLevel"] = LogLevelToString(levelFilter->GetMinLevel());
        config["maxLevel"] = LogLevelToString(levelFilter->GetMaxLevel());
        config["hasMaxLevel"] = levelFilter->HasMaxLevel();
        return config;
    };
    
    info.deserializer = [](ILogFilter* filter, const nlohmann::json& config) {
        LevelFilter* levelFilter = dynamic_cast<LevelFilter*>(filter);
        if (!levelFilter) {
            return;
        }
        
        if (config.contains("minLevel")) {
            LogLevel minLevel = StringToLogLevel(config["minLevel"].get<std::string>());
            levelFilter->SetMinLevel(minLevel);
        }
        
        if (config.contains("maxLevel") && config.contains("hasMaxLevel")) {
            bool hasMaxLevel = config["hasMaxLevel"].get<bool>();
            if (hasMaxLevel) {
                LogLevel maxLevel = StringToLogLevel(config["maxLevel"].get<std::string>());
                levelFilter->SetMaxLevel(maxLevel);
            }
        }
    };
    
    return info;
}

FilterTypeInfo LogFilterFactory::CreateKeywordFilterInfo() {
    FilterTypeInfo info;
    info.typeName = L"Keyword";
    info.description = L"Filter logs by keywords";
    info.isBuiltin = true;
    
    info.creator = []() -> std::unique_ptr<ILogFilter> {
        return std::make_unique<KeywordFilter>();
    };
    
    info.serializer = [](const ILogFilter* filter) -> nlohmann::json {
        const KeywordFilter* keywordFilter = dynamic_cast<const KeywordFilter*>(filter);
        if (!keywordFilter) {
            return nlohmann::json::object();
        }
        
        nlohmann::json config;
        config["caseSensitive"] = keywordFilter->IsCaseSensitive();
        
        nlohmann::json includeKeywords = nlohmann::json::array();
        for (const auto& keyword : keywordFilter->GetIncludeKeywords()) {
            includeKeywords.push_back(WStringToString(keyword));
        }
        config["includeKeywords"] = includeKeywords;
        
        nlohmann::json excludeKeywords = nlohmann::json::array();
        for (const auto& keyword : keywordFilter->GetExcludeKeywords()) {
            excludeKeywords.push_back(WStringToString(keyword));
        }
        config["excludeKeywords"] = excludeKeywords;
        
        return config;
    };
    
    info.deserializer = [](ILogFilter* filter, const nlohmann::json& config) {
        KeywordFilter* keywordFilter = dynamic_cast<KeywordFilter*>(filter);
        if (!keywordFilter) {
            return;
        }
        
        if (config.contains("caseSensitive")) {
            keywordFilter->SetCaseSensitive(config["caseSensitive"].get<bool>());
        }
        
        if (config.contains("includeKeywords")) {
            keywordFilter->ClearIncludeKeywords();
            for (const auto& item : config["includeKeywords"]) {
                if (item.is_string()) {
                    keywordFilter->AddIncludeKeyword(StringToWString(item.get<std::string>()));
                }
            }
        }
        
        if (config.contains("excludeKeywords")) {
            keywordFilter->ClearExcludeKeywords();
            for (const auto& item : config["excludeKeywords"]) {
                if (item.is_string()) {
                    keywordFilter->AddExcludeKeyword(StringToWString(item.get<std::string>()));
                }
            }
        }
    };
    
    return info;
}

FilterTypeInfo LogFilterFactory::CreateRegexFilterInfo() {
    FilterTypeInfo info;
    info.typeName = L"Regex";
    info.description = L"Filter logs using regular expressions";
    info.isBuiltin = true;
    
    info.creator = []() -> std::unique_ptr<ILogFilter> {
        return std::make_unique<RegexFilter>();
    };
    
    info.serializer = [](const ILogFilter* filter) -> nlohmann::json {
        const RegexFilter* regexFilter = dynamic_cast<const RegexFilter*>(filter);
        if (!regexFilter) {
            return nlohmann::json::object();
        }
        
        nlohmann::json config;
        config["pattern"] = WStringToString(regexFilter->GetPattern());
        config["isValid"] = regexFilter->IsPatternValid();
        return config;
    };
    
    info.deserializer = [](ILogFilter* filter, const nlohmann::json& config) {
        RegexFilter* regexFilter = dynamic_cast<RegexFilter*>(filter);
        if (!regexFilter) {
            return;
        }
        
        if (config.contains("pattern")) {
            std::wstring pattern = StringToWString(config["pattern"].get<std::string>());
            regexFilter->SetPattern(pattern);
        }
    };
    
    return info;
}

FilterTypeInfo LogFilterFactory::CreateFrequencyFilterInfo() {
    FilterTypeInfo info;
    info.typeName = L"RateLimit";  // 使用实际存在的类名
    info.description = L"Filter logs by rate limiting";
    info.isBuiltin = true;
    
    info.creator = []() -> std::unique_ptr<ILogFilter> {
        return std::make_unique<RateLimitFilter>();
    };
    
    info.serializer = [](const ILogFilter* filter) -> nlohmann::json {
        const RateLimitFilter* rateLimitFilter = dynamic_cast<const RateLimitFilter*>(filter);
        if (!rateLimitFilter) {
            return nlohmann::json::object();
        }
        
        nlohmann::json config;
        config["maxPerSecond"] = rateLimitFilter->GetMaxPerSecond();
        config["maxBurst"] = rateLimitFilter->GetMaxBurst();
        config["availableTokens"] = rateLimitFilter->GetAvailableTokens();
        return config;
    };
    
    info.deserializer = [](ILogFilter* filter, const nlohmann::json& config) {
        RateLimitFilter* rateLimitFilter = dynamic_cast<RateLimitFilter*>(filter);
        if (!rateLimitFilter) {
            return;
        }
        
        if (config.contains("maxPerSecond") && config.contains("maxBurst")) {
            size_t maxPerSecond = config["maxPerSecond"].get<size_t>();
            size_t maxBurst = config["maxBurst"].get<size_t>();
            rateLimitFilter->SetRateLimit(maxPerSecond, maxBurst);
        }
    };
    
    return info;
}

FilterTypeInfo LogFilterFactory::CreateThreadFilterInfo() {
    FilterTypeInfo info;
    info.typeName = L"Thread";
    info.description = L"Filter logs by thread ID";
    info.isBuiltin = true;
    
    info.creator = []() -> std::unique_ptr<ILogFilter> {
        return std::make_unique<ThreadFilter>();
    };
    
    info.serializer = [](const ILogFilter* filter) -> nlohmann::json {
        const ThreadFilter* threadFilter = dynamic_cast<const ThreadFilter*>(filter);
        if (!threadFilter) {
            return nlohmann::json::object();
        }
        
        nlohmann::json config;
        config["useAllowList"] = threadFilter->IsUsingAllowList();
        
        // 注意：由于ThreadFilter类没有提供获取线程列表的方法，
        // 这里只能序列化基本配置，具体的线程ID需要在使用时设置
        config["description"] = "Thread filter configuration - specific thread IDs need to be set at runtime";
        
        return config;
    };
    
    info.deserializer = [](ILogFilter* filter, const nlohmann::json& config) {
        ThreadFilter* threadFilter = dynamic_cast<ThreadFilter*>(filter);
        if (!threadFilter) {
            return;
        }
        
        if (config.contains("useAllowList")) {
            threadFilter->SetUseAllowList(config["useAllowList"].get<bool>());
        }
        
        // 注意：实际的线程ID添加需要在运行时进行，
        // 因为std::thread::id很难从字符串反序列化
    };
    
    return info;
}

// 辅助函数实现
std::string LogFilterFactory::WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) {
        return std::string();
    }
    return UniConv::GetInstance()->WideStringToLocale(wstr);
}

std::wstring LogFilterFactory::StringToWString(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }
    return UniConv::GetInstance()->LocaleToWideString(str);
}

std::string LogFilterFactory::LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "Trace";
        case LogLevel::Debug: return "Debug";
        case LogLevel::Info: return "Info";
        case LogLevel::Notice: return "Notice";
        case LogLevel::Warning: return "Warning";
        case LogLevel::Error: return "Error";
        case LogLevel::Critical: return "Critical";
        case LogLevel::Alert: return "Alert";
        case LogLevel::Emergency: return "Emergency";
        case LogLevel::Fatal: return "Fatal";
        default: return "Info";
    }
}

LogLevel LogFilterFactory::StringToLogLevel(const std::string& str) {
    if (str == "Trace") return LogLevel::Trace;
    if (str == "Debug") return LogLevel::Debug;
    if (str == "Info") return LogLevel::Info;
    if (str == "Notice") return LogLevel::Notice;
    if (str == "Warning") return LogLevel::Warning;
    if (str == "Error") return LogLevel::Error;
    if (str == "Critical") return LogLevel::Critical;
    if (str == "Alert") return LogLevel::Alert;
    if (str == "Emergency") return LogLevel::Emergency;
    if (str == "Fatal") return LogLevel::Fatal;
    return LogLevel::Info;
}