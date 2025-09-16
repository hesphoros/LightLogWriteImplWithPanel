#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <functional>

/**
 * @brief Enum for log levels
 * @details
 * TRACE      : 最详细的日志，通常用于跟踪程序执行的每一步。
 * DEBUG      : 调试信息，开发阶段使用，便于定位问题。
 * INFO       : 普通运行信息，程序正常运行时的重要事件。
 * NOTICE     : 需要注意但不是错误的事件。
 * WARNING    : 警告信息，潜在问题但不影响程序继续运行。
 * ERROR      : 错误信息，程序遇到问题但还能继续运行。
 * CRITICAL   : 严重错误，影响部分功能。
 * ALERT      : 需要立即关注的问题。
 * EMERGENCY  : 系统不可用或崩溃。
 * FATAL      : 致命错误，程序即将终止。
 */
enum class LogLevel {
	Trace = 0,  /*<! Detailed information, typically of interest only when diagnosing problems. */
	Debug, 		/*<! Fine-grained informational events that are most useful to debug an application. */
	Info, 		/*<! Informational messages that highlight the progress of the application at a coarse-grained level. */
	Notice, 	/*<! Events that are noteworthy but not error conditions. */
	Warning, 	/*<! Potentially harmful situations which still allow the application to continue running. */
	Error, 		/*<! Error events that might still allow the application to continue running. */
	Critical, 	/*<! Critical conditions that require immediate attention. */
	Alert, 		/*<! Action must be taken immediately. */
	Emergency, 	/*<! System is unusable. */
	Fatal 		/*<! Severe errors that lead to application termination. */
};

/**
 * @brief Structure for log callback information
 * @details Contains all information about a log event for callback notification
 */
struct LogCallbackInfo {
	LogLevel level;                        /*!< Log level */
	std::wstring levelString;              /*!< Log level as formatted string */
	std::wstring message;                  /*!< Log message content */
	std::chrono::system_clock::time_point timestamp; /*!< Timestamp when log was created */
	std::wstring formattedTime;            /*!< Formatted timestamp string */
	std::thread::id threadId;              /*!< ID of the thread that generated the log */
};

/**
 * @brief Log callback function type
 * @param callbackInfo Information about the log event
 * @details This function type is used for log event callbacks.
 *          The callback will be called on the logging thread, so implementations
 *          should be thread-safe and avoid blocking operations.
 */
using LogCallback = std::function<void(const LogCallbackInfo& callbackInfo)>;

/**
 * @brief Callback subscription handle
 * @details Used to identify and manage callback subscriptions
 */
using CallbackHandle = size_t;