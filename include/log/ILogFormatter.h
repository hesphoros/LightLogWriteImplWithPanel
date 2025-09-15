#pragma once

#include <string>
#include <memory>
#include <map>
#include <chrono>
#include <functional>

// Forward declarations
struct LogCallbackInfo;
enum class LogLevel;

/**
 * @brief Log format pattern tokens
 * @details Defines the available tokens that can be used in log format patterns
 */
enum class LogFormatToken {
    Timestamp,      // Date and time
    Level,          // Log level (INFO, ERROR, etc.)
    Message,        // Log message content
    ThreadId,       // Thread identifier
    FileName,       // Source file name
    LineNumber,     // Source line number
    FunctionName,   // Function name
    ProcessId,      // Process identifier
    LoggerName,     // Logger instance name
    NewLine,        // New line character
    Tab             // Tab character
};

/**
 * @brief Color code enumeration for console outputs
 */
enum class LogColor {
    Default,    // System default color
    Black,      // Black
    Red,        // Red
    Green,      // Green
    Yellow,     // Yellow
    Blue,       // Blue
    Magenta,    // Magenta
    Cyan,       // Cyan
    White,      // White
    Gray,       // Gray
    BrightRed,  // Bright red
    BrightGreen,// Bright green
    BrightYellow,// Bright yellow
    BrightBlue, // Bright blue
    BrightMagenta,// Bright magenta
    BrightCyan, // Bright cyan
    BrightWhite,// Bright white
    BgRed,      // Red background
    BgGreen,    // Green background
    BgYellow,   // Yellow background
    BgBlue,     // Blue background
    BgMagenta,  // Magenta background
    BgCyan,     // Cyan background
    BgWhite,    // White background
    Error       // Error color (implementation defined)
};

/**
 * @brief Log format configuration structure
 */
struct LogFormatConfig {
    std::wstring pattern = L"[{timestamp}] [{level}] {message}";  // Format pattern
    std::wstring timestampFormat = L"%Y-%m-%d %H:%M:%S";         // Timestamp format
    bool enableColors = false;                                    // Enable color output
    std::map<LogLevel, LogColor> levelColors;                    // Color mapping for levels
    bool enableThreadId = false;                                 // Include thread ID
    bool enableProcessId = false;                                // Include process ID
    bool enableSourceInfo = false;                              // Include file/line info
};

/**
 * @brief Abstract log formatter interface
 * @details Formats log entries according to configured patterns and styles
 */
class ILogFormatter {
public:
    virtual ~ILogFormatter() = default;

    /**
     * @brief Format a log entry
     * @param logInfo The log information to format
     * @return Formatted log string
     */
    virtual std::wstring FormatLog(const LogCallbackInfo& logInfo) = 0;

    /**
     * @brief Set the format configuration
     * @param config Format configuration
     */
    virtual void SetConfig(const LogFormatConfig& config) = 0;

    /**
     * @brief Get the current format configuration
     * @return Current configuration
     */
    virtual LogFormatConfig GetConfig() const = 0;

    /**
     * @brief Get the formatter type name
     * @return Type name string
     */
    virtual std::wstring GetFormatterType() const = 0;
};

// Type aliases
using LogFormatterPtr = std::shared_ptr<ILogFormatter>;

/**
 * @brief Factory function type for creating formatters
 */
using LogFormatterFactory = std::function<LogFormatterPtr(const LogFormatConfig& config)>;