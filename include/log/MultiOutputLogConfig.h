#pragma once

#include "../nlohmann/json.hpp"
#include "ILogOutput.h"
#include "ILogFormatter.h"
#include "ILogFilter.h"
#include "LogOutputManager.h"
#include <vector>
#include <string>
#include <memory>

/**
 * @brief Configuration for a single output
 */
struct OutputConfig {
    std::wstring name;                  // Output name
    std::wstring type;                  // Output type (Console, File, Network, etc.)
    bool enabled = true;                // Whether output is enabled
    LogLevel minLevel = LogLevel::Trace; // Minimum log level
    std::wstring config;                // Output-specific configuration

    // Formatter configuration
    bool useFormatter = true;
    LogFormatConfig formatterConfig;

    // Filter configuration
    bool useFilter = false;
    std::wstring filterType;            // Filter type (Level, Keyword, Regex, etc.)
    std::wstring filterConfig;          // Filter-specific configuration (JSON string)

    OutputConfig() = default;
    OutputConfig(const std::wstring& n, const std::wstring& t) : name(n), type(t) {}
};

/**
 * @brief Complete multi-output logging configuration
 */
struct MultiOutputLogConfig {
    // Manager configuration
    LogOutputManagerConfig managerConfig;
    
    // Output configurations
    std::vector<OutputConfig> outputs;
    
    // Global settings
    bool enabled = true;                // Global enable/disable
    LogLevel globalMinLevel = LogLevel::Trace; // Global minimum level
    std::wstring configVersion = L"1.0"; // Configuration version for compatibility

    MultiOutputLogConfig() = default;
};

/**
 * @brief JSON serialization/deserialization for multi-output configuration
 */
class MultiOutputConfigSerializer {
public:
    /**
     * @brief Serialize configuration to JSON
     * @param config Configuration to serialize
     * @return JSON object
     */
    static nlohmann::json ToJson(const MultiOutputLogConfig& config);

    /**
     * @brief Deserialize configuration from JSON
     * @param json JSON object to deserialize
     * @return Deserialized configuration
     */
    static MultiOutputLogConfig FromJson(const nlohmann::json& json);

    /**
     * @brief Save configuration to file
     * @param config Configuration to save
     * @param filePath File path to save to
     * @return true if successful
     */
    static bool SaveToFile(const MultiOutputLogConfig& config, const std::wstring& filePath);

    /**
     * @brief Load configuration from file
     * @param filePath File path to load from
     * @param config [out] Loaded configuration
     * @return true if successful
     */
    static bool LoadFromFile(const std::wstring& filePath, MultiOutputLogConfig& config);

private:
    // Helper methods for serialization
    static nlohmann::json SerializeOutputConfig(const OutputConfig& output);
    static OutputConfig DeserializeOutputConfig(const nlohmann::json& json);
    static nlohmann::json SerializeManagerConfig(const LogOutputManagerConfig& config);
    static LogOutputManagerConfig DeserializeManagerConfig(const nlohmann::json& json);
    static nlohmann::json SerializeFormatterConfig(const LogFormatConfig& config);
    static LogFormatConfig DeserializeFormatterConfig(const nlohmann::json& json);
    static nlohmann::json SerializeFilterConfig(const OutputConfig& output);
    static void DeserializeFilterConfig(const nlohmann::json& json, OutputConfig& output);
    
    // Utility methods
    static std::string WStringToString(const std::wstring& wstr);
    static std::wstring StringToWString(const std::string& str);
    static std::string LogLevelToString(LogLevel level);
    static LogLevel StringToLogLevel(const std::string& str);
    static std::string OutputWriteModeToString(OutputWriteMode mode);
    static OutputWriteMode StringToOutputWriteMode(const std::string& str);
};