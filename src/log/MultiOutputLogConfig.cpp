#define _CRT_SECURE_NO_WARNINGS
#include "log/MultiOutputLogConfig.h"
#include "log/LogFilterFactory.h"
#include <fstream>
#include <codecvt>
#include <locale>

// ========== Main Serialization Functions ==========

nlohmann::json MultiOutputConfigSerializer::ToJson(const MultiOutputLogConfig& config) {
    nlohmann::json j;
    
    // Basic configuration
    j["configVersion"] = WStringToString(config.configVersion);
    j["enabled"] = config.enabled;
    j["globalMinLevel"] = LogLevelToString(config.globalMinLevel);
    
    // Manager configuration
    j["managerConfig"] = SerializeManagerConfig(config.managerConfig);
    
    // Output configurations
    nlohmann::json outputs = nlohmann::json::array();
    for (const auto& output : config.outputs) {
        outputs.push_back(SerializeOutputConfig(output));
    }
    j["outputs"] = outputs;
    
    return j;
}

MultiOutputLogConfig MultiOutputConfigSerializer::FromJson(const nlohmann::json& json) {
    MultiOutputLogConfig config;
    
    try {
        // Basic configuration
        if (json.contains("configVersion")) {
            config.configVersion = StringToWString(json["configVersion"].get<std::string>());
        }
        
        if (json.contains("enabled")) {
            config.enabled = json["enabled"].get<bool>();
        }
        
        if (json.contains("globalMinLevel")) {
            config.globalMinLevel = StringToLogLevel(json["globalMinLevel"].get<std::string>());
        }
        
        // Manager configuration
        if (json.contains("managerConfig")) {
            config.managerConfig = DeserializeManagerConfig(json["managerConfig"]);
        }
        
        // Output configurations
        if (json.contains("outputs") && json["outputs"].is_array()) {
            config.outputs.clear();
            for (const auto& outputJson : json["outputs"]) {
                config.outputs.push_back(DeserializeOutputConfig(outputJson));
            }
        }
    }
    catch (const std::exception&) {
        // Return default config on error
        config = MultiOutputLogConfig{};
    }
    
    return config;
}

bool MultiOutputConfigSerializer::SaveToFile(const MultiOutputLogConfig& config, const std::wstring& filePath) {
    try {
        nlohmann::json j = ToJson(config);
        
        std::ofstream file(WStringToString(filePath));
        if (!file.is_open()) {
            return false;
        }
        
        file << j.dump(4); // Pretty print with 4-space indentation
        file.close();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool MultiOutputConfigSerializer::LoadFromFile(const std::wstring& filePath, MultiOutputLogConfig& config) {
    try {
        std::ifstream file(WStringToString(filePath));
        if (!file.is_open()) {
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        file.close();
        
        config = FromJson(j);
        return true;
    }
    catch (...) {
        return false;
    }
}

// ========== Helper Serialization Functions ==========

nlohmann::json MultiOutputConfigSerializer::SerializeOutputConfig(const OutputConfig& output) {
    nlohmann::json j;
    
    j["name"] = WStringToString(output.name);
    j["type"] = WStringToString(output.type);
    j["enabled"] = output.enabled;
    j["minLevel"] = LogLevelToString(output.minLevel);
    j["config"] = WStringToString(output.config);
    
    // Formatter configuration
    j["useFormatter"] = output.useFormatter;
    j["formatterConfig"] = SerializeFormatterConfig(output.formatterConfig);
    
    // Filter configuration
    j["useFilter"] = output.useFilter;
    if (output.useFilter) {
        j["filterConfig"] = SerializeFilterConfig(output);
    }
    
    return j;
}

OutputConfig MultiOutputConfigSerializer::DeserializeOutputConfig(const nlohmann::json& json) {
    OutputConfig output;
    
    try {
        if (json.contains("name")) {
            output.name = StringToWString(json["name"].get<std::string>());
        }
        
        if (json.contains("type")) {
            output.type = StringToWString(json["type"].get<std::string>());
        }
        
        if (json.contains("enabled")) {
            output.enabled = json["enabled"].get<bool>();
        }
        
        if (json.contains("minLevel")) {
            output.minLevel = StringToLogLevel(json["minLevel"].get<std::string>());
        }
        
        if (json.contains("config")) {
            output.config = StringToWString(json["config"].get<std::string>());
        }
        
        // Formatter configuration
        if (json.contains("useFormatter")) {
            output.useFormatter = json["useFormatter"].get<bool>();
        }
        
        if (json.contains("formatterConfig")) {
            output.formatterConfig = DeserializeFormatterConfig(json["formatterConfig"]);
        }
        
        // Filter configuration
        if (json.contains("useFilter")) {
            output.useFilter = json["useFilter"].get<bool>();
        }
        if (output.useFilter && json.contains("filterConfig")) {
            DeserializeFilterConfig(json["filterConfig"], output);
        }
    }
    catch (const std::exception&) {
        // Return default on error
        output = OutputConfig{};
    }
    
    return output;
}

nlohmann::json MultiOutputConfigSerializer::SerializeManagerConfig(const LogOutputManagerConfig& config) {
    nlohmann::json j;
    
    j["writeMode"] = OutputWriteModeToString(config.writeMode);
    j["asyncQueueSize"] = config.asyncQueueSize;
    j["workerThreadCount"] = config.workerThreadCount;
    j["failFastOnError"] = config.failFastOnError;
    j["writeTimeout"] = config.writeTimeout;
    
    return j;
}

LogOutputManagerConfig MultiOutputConfigSerializer::DeserializeManagerConfig(const nlohmann::json& json) {
    LogOutputManagerConfig config;
    
    try {
        if (json.contains("writeMode")) {
            config.writeMode = StringToOutputWriteMode(json["writeMode"].get<std::string>());
        }
        
        if (json.contains("asyncQueueSize")) {
            config.asyncQueueSize = json["asyncQueueSize"].get<size_t>();
        }
        
        if (json.contains("workerThreadCount")) {
            config.workerThreadCount = json["workerThreadCount"].get<size_t>();
        }
        
        if (json.contains("failFastOnError")) {
            config.failFastOnError = json["failFastOnError"].get<bool>();
        }
        
        if (json.contains("writeTimeout")) {
            config.writeTimeout = json["writeTimeout"].get<double>();
        }
    }
    catch (const std::exception&) {
        // Return default on error
        config = LogOutputManagerConfig{};
    }
    
    return config;
}

nlohmann::json MultiOutputConfigSerializer::SerializeFormatterConfig(const LogFormatConfig& config) {
    nlohmann::json j;
    
    j["pattern"] = WStringToString(config.pattern);
    j["timestampFormat"] = WStringToString(config.timestampFormat);
    j["enableColors"] = config.enableColors;
    j["enableThreadId"] = config.enableThreadId;
    j["enableProcessId"] = config.enableProcessId;
    j["enableSourceInfo"] = config.enableSourceInfo;
    
    // Serialize level colors map
    nlohmann::json levelColors;
    for (const auto& pair : config.levelColors) {
        levelColors[LogLevelToString(pair.first)] = static_cast<int>(pair.second);
    }
    j["levelColors"] = levelColors;
    
    return j;
}

LogFormatConfig MultiOutputConfigSerializer::DeserializeFormatterConfig(const nlohmann::json& json) {
    LogFormatConfig config;
    
    try {
        if (json.contains("pattern")) {
            config.pattern = StringToWString(json["pattern"].get<std::string>());
        }
        
        if (json.contains("timestampFormat")) {
            config.timestampFormat = StringToWString(json["timestampFormat"].get<std::string>());
        }
        
        if (json.contains("enableColors")) {
            config.enableColors = json["enableColors"].get<bool>();
        }
        
        if (json.contains("enableThreadId")) {
            config.enableThreadId = json["enableThreadId"].get<bool>();
        }
        
        if (json.contains("enableProcessId")) {
            config.enableProcessId = json["enableProcessId"].get<bool>();
        }
        
        if (json.contains("enableSourceInfo")) {
            config.enableSourceInfo = json["enableSourceInfo"].get<bool>();
        }
        
        // Deserialize level colors map
        if (json.contains("levelColors") && json["levelColors"].is_object()) {
            config.levelColors.clear();
            for (const auto& item : json["levelColors"].items()) {
                LogLevel level = StringToLogLevel(item.key());
                LogColor color = static_cast<LogColor>(item.value().get<int>());
                config.levelColors[level] = color;
            }
        }
    }
    catch (const std::exception&) {
        // Return default on error
        config = LogFormatConfig{};
    }
    
    return config;
}

// ========== Utility Functions ==========

std::string MultiOutputConfigSerializer::WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wstr);
    }
    catch (...) {
        // Fallback: simple conversion (may lose non-ASCII characters)
        return std::string(wstr.begin(), wstr.end());
    }
}

std::wstring MultiOutputConfigSerializer::StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(str);
    }
    catch (...) {
        // Fallback: simple conversion (may not work correctly for UTF-8)
        return std::wstring(str.begin(), str.end());
    }
}

std::string MultiOutputConfigSerializer::LogLevelToString(LogLevel level) {
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

LogLevel MultiOutputConfigSerializer::StringToLogLevel(const std::string& str) {
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
    return LogLevel::Info; // Default
}

std::string MultiOutputConfigSerializer::OutputWriteModeToString(OutputWriteMode mode) {
    switch (mode) {
        case OutputWriteMode::Sequential: return "Sequential";
        case OutputWriteMode::Parallel: return "Parallel";
        case OutputWriteMode::Async: return "Async";
        default: return "Sequential";
    }
}

OutputWriteMode MultiOutputConfigSerializer::StringToOutputWriteMode(const std::string& str) {
    if (str == "Sequential") return OutputWriteMode::Sequential;
    if (str == "Parallel") return OutputWriteMode::Parallel;
    if (str == "Async") return OutputWriteMode::Async;
    return OutputWriteMode::Sequential; // Default
}

nlohmann::json MultiOutputConfigSerializer::SerializeFilterConfig(const OutputConfig& output) {
    nlohmann::json filterConfig;
    
    if (!output.filterType.empty()) {
        filterConfig["type"] = WStringToString(output.filterType);
    }
    
    if (!output.filterConfig.empty()) {
        try {
            // Try to parse filterConfig as JSON, if it fails, store as string
            filterConfig["config"] = nlohmann::json::parse(WStringToString(output.filterConfig));
        }
        catch (...) {
            // If not valid JSON, store as string
            filterConfig["config"] = WStringToString(output.filterConfig);
        }
    }
    
    return filterConfig;
}

void MultiOutputConfigSerializer::DeserializeFilterConfig(const nlohmann::json& json, OutputConfig& output) {
    try {
        if (json.contains("type")) {
            output.filterType = StringToWString(json["type"].get<std::string>());
        }
        
        if (json.contains("config")) {
            if (json["config"].is_string()) {
                output.filterConfig = StringToWString(json["config"].get<std::string>());
            } else {
                // If it's a JSON object, serialize it back to string
                output.filterConfig = StringToWString(json["config"].dump());
            }
        }
    }
    catch (const std::exception&) {
        // Clear filter config on error
        output.filterType.clear();
        output.filterConfig.clear();
    }
}