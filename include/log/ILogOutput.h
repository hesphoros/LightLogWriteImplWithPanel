#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <functional>

// Forward declarations
enum class LogLevel;
struct LogCallbackInfo;
class ILogFormatter;
class ILogFilter;

/**
 * @brief Output result enumeration
 * @details Represents the result of a log output operation
 */
enum class LogOutputResult {
    Success,        // Log was successfully written
    Failed,         // Log write operation failed
    Filtered,       // Log was filtered out by filter rules
    Unavailable     // Output is not available (disabled, disconnected, etc.)
};

/**
 * @brief Output statistics structure
 * @details Tracks performance and usage statistics for each output
 */
struct LogOutputStats {
    size_t totalLogs = 0;           // Total number of logs processed
    size_t successfulLogs = 0;      // Number of successfully written logs
    size_t failedLogs = 0;          // Number of failed log writes
    size_t filteredLogs = 0;        // Number of filtered logs
    std::chrono::system_clock::time_point lastWriteTime;  // Last successful write time
    double averageWriteTime = 0.0;  // Average write time in milliseconds
    size_t bytesWritten = 0;        // Total bytes written to this output
};

/**
 * @brief Abstract base interface for all log outputs
 * @details This interface defines the contract that all log output implementations must follow.
 *          It provides a unified API for writing logs to different destinations like files,
 *          console, network, database, etc.
 */
class ILogOutput {
public:
    virtual ~ILogOutput() = default;

    // Core logging functionality
    /**
     * @brief Write a log entry to this output
     * @param logInfo The log information to write
     * @return Result of the write operation
     */
    virtual LogOutputResult WriteLog(const LogCallbackInfo& logInfo) = 0;

    /**
     * @brief Flush any buffered data to the output destination
     */
    virtual void Flush() = 0;

    /**
     * @brief Check if this output is currently available for writing
     * @return true if available, false otherwise
     */
    virtual bool IsAvailable() const = 0;

    // Configuration and management
    /**
     * @brief Initialize the output with given configuration
     * @param config Configuration string (format depends on implementation)
     * @return true if initialization succeeded
     */
    virtual bool Initialize(const std::wstring& config = L"") = 0;

    /**
     * @brief Shutdown the output and cleanup resources
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Get current configuration as a string
     * @return Configuration string
     */
    virtual std::wstring GetConfigString() const = 0;

    // Properties
    /**
     * @brief Get the name of this output
     * @return Output name
     */
    virtual std::wstring GetOutputName() const = 0;

    /**
     * @brief Get the type of this output (e.g., "File", "Console", "Network")
     * @return Output type string
     */
    virtual std::wstring GetOutputType() const = 0;

    // Formatter and filter management
    /**
     * @brief Set the log formatter for this output
     * @param formatter Formatter to use (nullptr to use default)
     */
    virtual void SetFormatter(std::shared_ptr<ILogFormatter> formatter) = 0;

    /**
     * @brief Get the current log formatter
     * @return Current formatter (may be nullptr)
     */
    virtual std::shared_ptr<ILogFormatter> GetFormatter() const = 0;

    /**
     * @brief Set the log filter for this output
     * @param filter Filter to use (nullptr for no filtering)
     */
    virtual void SetFilter(std::shared_ptr<ILogFilter> filter) = 0;

    /**
     * @brief Get the current log filter
     * @return Current filter (may be nullptr)
     */
    virtual std::shared_ptr<ILogFilter> GetFilter() const = 0;

    // Statistics and monitoring
    /**
     * @brief Get statistics for this output
     * @return Statistics structure
     */
    virtual LogOutputStats GetStatistics() const = 0;

    /**
     * @brief Reset statistics counters
     */
    virtual void ResetStatistics() = 0;

    // Level and enable/disable control
    /**
     * @brief Set minimum log level for this output
     * @param minLevel Minimum level to process
     */
    virtual void SetMinLogLevel(LogLevel minLevel) = 0;

    /**
     * @brief Get minimum log level for this output
     * @return Current minimum log level
     */
    virtual LogLevel GetMinLogLevel() const = 0;

    /**
     * @brief Enable or disable this output
     * @param enabled true to enable, false to disable
     */
    virtual void SetEnabled(bool enabled) = 0;

    /**
     * @brief Check if this output is enabled
     * @return true if enabled, false otherwise
     */
    virtual bool IsEnabled() const = 0;
};

// Type aliases for convenience
using LogOutputPtr = std::shared_ptr<ILogOutput>;

/**
 * @brief Factory function type for creating log outputs
 */
using LogOutputFactory = std::function<LogOutputPtr(const std::wstring& config)>;