#ifndef INCLUDE_LIGHTLOGWRITEIMPL_HPP_
#define INCLUDE_LIGHTLOGWRITEIMPL_HPP_
#define NOMINMAX
/*****************************************************************************
 *  LightLogWriteImpl
 *  Copyright (C) 2025 hesphoros <hesphoros@gmail.com>
 *
 *  This file is part of LightLogWriteImpl.
 *
 *  This program is free software; you can redistribute it d:\codespace\LightLogWriteImpl\include\LightLogWriteImpl.hppand/or modify
 *  it under the terms of the GNU General Public License version 3 as
 *  published by the Free Software Foundation.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *  @file     LightLogWriteImpl.hpp
 *  @brief    有锁实现的轻量级的日志库
 *  @details  详细描述
 *
 *  @author   hesphoros
 *  @email    hesphoros@gmail.com
 *  @version  1.0.0.1
 *  @date     2025/05/27
 *  @license  GNU General Public License (GPL)
 *---------------------------------------------------------------------------*
 *  Remark         : None
 *---------------------------------------------------------------------------*
 *  Change History :
 *  <Date>     | <Version> | <Author>       | <Description>
 *  2025/03/27 | 1.0.0.1   | hesphoros      | Create file
 *****************************************************************************/

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <locale>
#include <codecvt>
#include <stdexcept>
#include <memory>
#include <functional>
#include <list>

// Forward declarations
class UniConv;
class LogOutputManager;
class ILogOutput;
class ILogCompressor;
class LogCompressor;
struct MultiOutputLogConfig;
struct CompressionStatistics;

struct LogCallbackInfo;

/**

  * @file LightLogWriteCommon.h
  * @brief Common definitions and structures for LightLogWrite.
  * This file contains the definitions of structures and enums used in the LightLogWrite library.
  * It includes the LightLogWriteInfo structure for log messages and the LogQueueOverflowStrategy enum for handling full log queues.
  * @note Need -std-cpp17
  */

  // to use iconv for character encoding conversion #pragma comment(lib, "libiconv.lib")

/**
   * @brief Structure for log message information.
   * @param sLogTagNameVal The tag name of the log.
   * * It can be used to categorize or identify the log message.
   * * such as INFO , WARNING, ERROR, etc.
   * @param sLogContentVal The content of the log message.
   * * It contains the actual log message that will be written to the log file.
   * * This can include any relevant information that needs to be logged, such as error messages, status updates, etc.
  */
struct LightLogWriteInfo {
	std::wstring                   sLogTagNameVal;  /*!< Log tag name */
	std::wstring                   sLogContentVal;  /*!< Log content */
};

/**
	* @brief Enum for strategies to handle full log queues.
	* @details
	* * This enum defines the strategies that can be used when the log queue is full.
	* * It provides options for blocking until space is available or dropping the oldest log entry.
	* @param Block Blocked waiting for space in the queue.
	* * When the queue is full, the logging operation will block until space becomes available.
	* @param DropOldest Drop the oldest log entry when the queue is full.
	* * When the queue is full, the oldest log entry will be removed to make space for the new log entry.
	* * This strategy allows for continuous logging without blocking, but may result in loss of older log entries.
	*/
enum class LogQueueOverflowStrategy {
	Block,      /*!< Blocked waiting           */
	DropOldest  /*!< Drop the oldest log entry */
};


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

/**
 * @brief Log rotation strategy enum
 */
enum class LogRotationStrategy {
	None,           /*!< No rotation */
	Size,           /*!< Rotate by file size */
	Time,           /*!< Rotate by time interval */
	SizeAndTime     /*!< Rotate by both size and time */
};

/**
 * @brief Time rotation interval enum
 */
enum class TimeRotationInterval {
	Hourly,         /*!< Rotate every hour */
	Daily,          /*!< Rotate every day */
	Weekly,         /*!< Rotate every week */
	Monthly         /*!< Rotate every month */
};

/**
 * @brief Log rotation configuration
 */
struct LogRotationConfig {
	LogRotationStrategy strategy = LogRotationStrategy::None;        /*!< Rotation strategy */
	size_t maxFileSizeMB = 10;                                      /*!< Max file size in MB */
	TimeRotationInterval timeInterval = TimeRotationInterval::Daily; /*!< Time rotation interval */
	size_t maxArchiveFiles = 30;                                     /*!< Max number of archive files to keep */
	bool enableCompression = true;                                   /*!< Enable compression for archived files */
	std::wstring archiveDirectory = L"";                             /*!< Archive directory (empty = same as log dir) */
};

/**
 * @brief Implementation of the LightLogWrite class
 * @details This class provides a thread-safe implementation for writing logs to a file.
*/
class LightLogWrite_Impl {
public:
	/**
		* @brief Constructor for LightLogWrite_Impl
		* @param maxQueueSize The max log written queue size
		* @param strategy The strategy for handling full log queue
		* @param reportInterval The interval for reporting log overflow
		* @details This constructor initializes the log writer with a specified maximum queue size,
		*          a strategy for handling full queues, and an interval for reporting log overflow.
		* @note The default maximum queue size is 500000, the default strategy is to block when the queue is full,
		*       and the default report interval is 100 discarded logs.
		* @version 1.0.0
		*/
	LightLogWrite_Impl(size_t maxQueueSize = 500000, LogQueueOverflowStrategy strategy = LogQueueOverflowStrategy::Block, size_t reportInterval = 100, std::shared_ptr<ILogCompressor> compressor = nullptr);

	/**
		* @brief Destructor for LightLogWrite_Impl
		* @details This destructor closes the log file stream and stops the logging thread.
		* * It will write a finial log entry indicating that the logging has stopped.
		* @note It is important to call this destructor to ensure that all logs are flushed and the thread is properly terminated.
		*/
	~LightLogWrite_Impl();

	/**
	* @brief Sets the log file name for logging
	* @param sFilename The name of the log file to be used for logging
	* @details This function sets the log file name and ensures that the directory exists.
	* * If the directory does not exist, it will be created.
	*/
	void SetLogsFileName(const std::wstring& sFilename);

	/**
		* @brief Sets the log file name for logging
		* @param sFilename The name of the log file to be used for logging
		* @details This function sets the log file name and ensures that the directory exists.
		* * If the directory does not exist, it will be created.
		*/
	void SetLogsFileName(const std::string& sFilename);


	void SetLogsFileName(const std::u16string& sFilename);

	/**
		* @brief Sets the directory and base name for lasting logs
		* @param sFilePath The directory path where the logs will be stored
		* @param sBaseName The base name for the log files
		* @details This function sets the directory and base name for lasting logs.
		* * It will create a new log file based on the current date and time.
		* * The log files will be named in the format "BaseName_YYYY_MM_DD_AM/PM.log". such as "LightLogWriteImpl_2025_05_27_AM.log".
		* * If the directory does not exist, it will be created.
		* @note This function should be called before writing any logs to ensure that the logs are stored correctly.
		*/
	void SetLastingsLogs(const std::wstring& sFilePath, const std::wstring& sBaseName);

	void SetLastingsLogs(const std::u16string& sFilePath, const std::u16string& sBaseName);


	void SetLastingsLogs(const std::string& sFilePath, const std::string& sBaseName);

	/**
	* @brief Writes a log message with a specific LogLevel
	* @param level The log level enum
	* @param sMessage The content of the log message
	* @details This function writes a log message with a specific log level to the log file.
	* It will also handle log overflow according to the specified strategy.
	*/
	void WriteLogContent(LogLevel level, const std::wstring& sMessage);
	void WriteLogContent(LogLevel level, const std::string& sMessage);

	/**
	* @brief Writes a log message with a specific type
	* @param sTypeVal The type of the log message (e.g., "INFO", "ERROR")
	* @param sMessage The content of the log message
	* @details This function writes a log message with a specific type to the log file.
	* It will also handle log overflow according to the specified strategy.
	*/
	void WriteLogContent(const std::wstring& sTypeVal, const std::wstring& sMessage);

	void WriteLogContent(const std::string& sTypeVal, const std::string& sMessage);


	void WriteLogContent(const std::u16string& sTypeVal, const std::u16string& sMessage);

	/**
	* @brief Gets the current discard count
	* @return The number of discarded log messages
	* @retval size_t The number of log messages that have been discarded due to overflow
	* @details This function returns the current count of discarded log messages.
	*/
	size_t GetDiscardCount() const;

	/**
	* @brief Resets the discard count to zero
	* @details This function resets the count of discarded log messages to zero.
	* * It can be useful for clearing the count after handling or reporting the discarded logs.
	* @note This function does not affect the log messages themselves, only the count of discarded messages.
	*/
	void ResetDiscardCount();

	/**
	 * @brief Subscribe to log events with a callback function
	 * @param callback The callback function to be called when a log event occurs
	 * @param minLevel The minimum log level to trigger the callback (default: Trace)
	 * @return CallbackHandle A handle that can be used to unsubscribe the callback
	 * @details This function registers a callback that will be called whenever a log message
	 *          is written. The callback will be executed on the logging thread, so it should
	 *          be thread-safe and avoid blocking operations.
	 * @note Callbacks are called after the log is queued but before it's written to file.
	 */
	CallbackHandle SubscribeToLogEvents(const LogCallback& callback, LogLevel minLevel = LogLevel::Trace);

	/**
	 * @brief Unsubscribe from log events
	 * @param handle The handle returned by SubscribeToLogEvents
	 * @return true if the callback was found and removed, false otherwise
	 * @details This function removes a previously registered callback.
	 */
	bool UnsubscribeFromLogEvents(CallbackHandle handle);

	/**
	 * @brief Remove all log event callbacks
	 * @details This function removes all registered callbacks.
	 */
	void ClearAllLogCallbacks();

	/**
	 * @brief Get the number of registered callbacks
	 * @return The number of currently registered callbacks
	 */
	size_t GetCallbackCount() const;

	/**
	 * @brief Configure log rotation settings
	 * @param config The rotation configuration
	 * @details This function sets up automatic log rotation based on size and/or time
	 */
	void SetLogRotationConfig(const LogRotationConfig& config);

	/**
	 * @brief Get current log rotation configuration
	 * @return The current rotation configuration
	 */
	LogRotationConfig GetLogRotationConfig() const;

	/**
	 * @brief Force log rotation immediately
	 * @details This function forces an immediate log rotation regardless of current settings
	 */
	void ForceLogRotation();

	/**
	 * @brief Get current log file size in bytes
	 * @return Current log file size, or 0 if no active log file
	 */
	size_t GetCurrentLogFileSize() const;

	/**
	 * @brief Clean up old archive files based on retention policy
	 * @details This function removes old archived log files according to the maxArchiveFiles setting
	 */
	void CleanupOldArchives();

		/**
	 * @brief Sets the minimum log level for logging
	 * @param level The minimum log level to be set
	 * @details This function sets the minimum log level for logging.
	 * * Log messages with a lower severity than this level will be ignored.
	 */
	void     SetMinLogLevel(LogLevel level) { eMinLogLevel = level; }
	LogLevel GetMinLogLevel() const { return eMinLogLevel; }

	// 宏定义用于简化重复的日志函数声明
	#define DECLARE_LOG_FUNCTIONS(FuncName) \
		void WriteLog##FuncName(const std::string& sMessage); \
		void WriteLog##FuncName(const std::wstring& sMessage);

	DECLARE_LOG_FUNCTIONS(Trace)
	DECLARE_LOG_FUNCTIONS(Debug)
	DECLARE_LOG_FUNCTIONS(Info)
	DECLARE_LOG_FUNCTIONS(Notice)
	DECLARE_LOG_FUNCTIONS(Warning)
	DECLARE_LOG_FUNCTIONS(Error)
	DECLARE_LOG_FUNCTIONS(Critical)
	DECLARE_LOG_FUNCTIONS(Alert)
	DECLARE_LOG_FUNCTIONS(Emergency)
	DECLARE_LOG_FUNCTIONS(Fatal)

	#undef DECLARE_LOG_FUNCTIONS

private:

	/**
	 * @brief Converts a LogLevel enum to its corresponding wide string representation
	 * @param level The LogLevel enum value to be converted
	 * @return A wide string representing the log level
	 * @details This function takes a LogLevel enum value and returns its corresponding wide string representation.
	 */
	std::wstring LogLevelToWString(LogLevel level) const;


	/**
	 * @brief Converts a LogLevel enum to its corresponding string representation
	 * @param level The LogLevel enum value to be converted
	 * @return A string representing the log level
	 * @details This function takes a LogLevel enum value and returns its corresponding string representation.
	 * * For example, LogLevel::Info will be converted to "INFO".
	 */
	std::string LogLevelToString(LogLevel level) const;
	
	/**
	* @brief Builds the output log file name based on the current date and time
	* @return A wide string representing the log file name
	* @details This function constructs the log file name using the current date and time.
	* * The format is "BaseName_YYYY_MM_DD_AM/PM.log", where BaseName is the base name set by SetLastingsLogs.
	*/
	std::wstring BuildLogFileOut();


	/**
	* @brief Closes the log stream and stops the logging thread
	* @details This function sets the stop flag for the logging thread, notifies it to wake up,
	* * and waits for the thread to finish.
	* * It also writes a final log entry indicating that the logging has stopped.
	* @note It is important to call this function to ensure that all logs are flushed and the thread is properly terminated.
	*/
	void CloseLogStream();

	/**
		* @brief Creates a new log file based on the current date and time
		* @details This function constructs a new log file name using the current date and time,
		* * checks if the directory exists, and opens the log file stream for appending.
		* * If the directory does not exist, it will be created.
		*/
	void CreateLogsFile();

	/**
		* @brief Creates a new log file without acquiring locks (for internal use)
		* @details Internal version that doesn't acquire locks, for use when caller already holds the lock
		*/
	void CreateLogsFileUnlocked();

	/**
		* @brief Runs the log writing thread
		* @details This function runs in a separate thread and continuously checks for new log messages to write.
		* * It waits for new log messages to be added to the queue and writes them to the log file.
		* * If the log file needs to be created or rotated, it will handle that as well.
		* * The thread will exit when the stop flag is set and the queue is empty.
		* @note This function should be called in a separate thread to avoid blocking the main application.
		*/
	void RunWriteThread();

	/**
		* @brief Checks if the directory for the log file exists, and creates it if it does not
		* @param sFilename The full path of the log file
		* @details This function checks if the directory of the specified log file exists.
		* * If the directory does not exist, it will create all necessary directories.
		*/
	void ChecksDirectory(const std::wstring& sFilename);

	/**
		* @brief Gets the current time as a formatted string
		* @return A wide string representing the current time in the format "YYYY-MM-DD HH:MM:SS"
		* @details This function retrieves the current system time and formats it into a wstring.
		* * The format used is "YYYY-MM-DD HH:MM:SS", which is suitable for logging purposes.
		*/
	std::wstring GetCurrentTimer() const;

	/**
		* @brief Gets the current time as a tm structure
		* @return A tm structure representing the current time
		* @details This function retrieves the current system time and converts it into a tm structure.
		* * The tm structure contains various components of the time, such as year, month, day, hour, minute, and second.
		* * This is useful for formatting or manipulating time data in a more structured way.
		* @note The tm structure is in local time, so it will reflect the local timezone settings of the system.
		* @retval std::tm The current time as a tm structure
		* @version 1.0.0
		*/
	std::tm GetCurrsTimerTm() const;

	/**
	 * @brief Triggers callbacks for a log event
	 * @param level The log level
	 * @param levelString The formatted log level string
	 * @param message The log message
	 * @details This function calls all registered callbacks that match the log level criteria.
	 *          It's called from the WriteLogContent method before queuing the log message.
	 */
	void TriggerLogCallbacks(LogLevel level, const std::wstring& levelString, const std::wstring& message);

	/**
	 * @brief Check if log rotation is needed and perform rotation if necessary
	 * @details This function checks rotation conditions and performs rotation if needed
	 */
	void CheckAndPerformRotation();

	/**
	 * @brief Perform actual log file rotation
	 * @param reason The reason for rotation (for logging purposes)
	 * @details This function performs the actual rotation process including archiving and compression
	 */
	void PerformLogRotation(const std::wstring& reason);

	/**
	 * @brief Generate archive file name with timestamp
	 * @param baseFileName The base file name
	 * @param timestamp The timestamp for the archive
	 * @return The generated archive file name
	 */
	std::wstring GenerateArchiveFileName(const std::wstring& baseFileName, const std::chrono::system_clock::time_point& timestamp);

	/**
	 * @brief Check if time-based rotation is needed
	 * @return true if time rotation is needed, false otherwise
	 */
	bool IsTimeRotationNeeded() const;

	/**
	 * @brief Get current log file size without locking (for internal use)
	 * @return Current log file size, or 0 if no active log file
	 * @details This version doesn't acquire locks, for use when caller already holds the lock
	 */
	size_t GetCurrentLogFileSizeUnlocked() const;

	// Compression system methods
public:
	/**
	 * @brief Set the log compressor instance
	 * @param compressor The compressor instance to use (can be nullptr to disable compression)
	 * @details This allows runtime replacement of the compression system
	 */
	void SetCompressor(std::shared_ptr<ILogCompressor> compressor);

	/**
	 * @brief Get the current compressor instance
	 * @return Pointer to the current compressor, or nullptr if no compressor is set
	 */
	std::shared_ptr<ILogCompressor> GetCompressor() const;

	/**
	 * @brief Get compression statistics
	 * @return Compression statistics from the current compressor
	 * @details Returns empty statistics if no compressor is available
	 */
	CompressionStatistics GetCompressionStatistics() const;

	// Multi-output system methods
public:
	/**
	 * @brief Save multi-output configuration to JSON file
	 * @param configFilePath Path to the configuration file
	 * @return true if successful, false otherwise
	 */
	bool SaveMultiOutputConfigToJson(const std::wstring& configFilePath);

	/**
	 * @brief Load multi-output configuration from JSON file
	 * @param configFilePath Path to the configuration file
	 * @return true if successful, false otherwise
	 */
	bool LoadMultiOutputConfigFromJson(const std::wstring& configFilePath);

	/**
	 * @brief Add a log output to the multi-output system
	 * @param output The output to add
	 * @return true if successful, false otherwise
	 */
	bool AddLogOutput(std::shared_ptr<ILogOutput> output);

	/**
	 * @brief Remove a log output from the multi-output system
	 * @param outputName Name of the output to remove
	 * @return true if successful, false otherwise
	 */
	bool RemoveLogOutput(const std::wstring& outputName);

	/**
	 * @brief Get the multi-output manager
	 * @return Pointer to the output manager
	 */
	std::shared_ptr<LogOutputManager> GetOutputManager() const;

	/**
	 * @brief Enable or disable multi-output system
	 * @param enabled Whether to enable multi-output
	 */
	void SetMultiOutputEnabled(bool enabled);

	/**
	 * @brief Check if multi-output system is enabled
	 * @return true if enabled, false otherwise
	 */
	bool IsMultiOutputEnabled() const;

private:

	static const wchar_t* LOG_LEVEL_STRINGS_W[];
	static const char*    LOG_LEVEL_STRINGS_A[];


	//------------------------------------------------------------------------------------------------
	// Section Name: Private Members @{                                                              +
	//------------------------------------------------------------------------------------------------
	std::wofstream                  pLogFileStream;            /*!< Log file stream                  */
	mutable std::mutex              pLogWriteMutex;            /*!< Log write mutex                  */
	std::queue<LightLogWriteInfo>   pLogWriteQueue;            /*!< Log write queue FIFO             */
	std::condition_variable         pWrittenCondVar;           /*!< Cond for waking log write thread */
	std::thread                     sWrittenThreads;           /*!< Log write thread                 */
	std::atomic<bool>               bIsStopLogging;            /*!< Stop flag                        */
	std::wstring                    sLogLastingDir;            /*!< Directory for lasting logs       */
	std::wstring                    sLogsBasedName;            /*!< Base name for log files          */
	std::atomic<bool>               bHasLogLasting;            /*!< Whether to persist logs          */
	std::atomic<bool>               bLastingTmTags;            /*!< Current log file AM/PM tag       */
	const size_t                    kMaxQueueSize;             /*!< Max queue size                   */
	LogQueueOverflowStrategy        queueFullStrategy;         /*!< Queue full strategy              */
	std::atomic<size_t>             discardCount;              /*!< Discard count                    */
	std::atomic<size_t>             lastReportedDiscardCount;  /*!< Last reported discard count      */
	std::atomic<size_t>             reportInterval;            /*!< Report interval                  */
	LogLevel                        eMinLogLevel;              /*!< Minimum log level[default: TRACE]*/
	
	// Callback management members
	struct CallbackEntry {
		CallbackHandle              handle;                    /*!< Unique handle for the callback   */
		LogCallback                 callback;                  /*!< The callback function            */
		LogLevel                    minLevel;                  /*!< Minimum level to trigger callback*/
	};
	std::list<CallbackEntry>        pLogCallbacks;             /*!< List of registered callbacks     */
	mutable std::mutex              pCallbackMutex;            /*!< Mutex for callback list          */
	std::atomic<CallbackHandle>     pNextCallbackHandle;       /*!< Next available callback handle   */
	
	// Log rotation members
	LogRotationConfig               rotationConfig;            /*!< Log rotation configuration       */
	std::wstring                    currentLogFileName;        /*!< Current active log file name     */
	std::chrono::system_clock::time_point lastRotationTime;    /*!< Last rotation timestamp          */
	mutable std::mutex              rotationMutex;             /*!< Mutex for rotation operations    */
	std::atomic<bool>               forceRotationRequested;    /*!< Manual rotation request flag     */
	
	// Compression system members
	std::shared_ptr<ILogCompressor> logCompressor_;           /*!< Log compressor instance          */
	mutable std::mutex              compressorMutex_;         /*!< Mutex for compressor operations  */
	
	// Multi-output system members
	std::shared_ptr<LogOutputManager> multiOutputManager;     /*!< Multi-output manager             */
	std::atomic<bool>               multiOutputEnabled;       /*!< Multi-output enabled flag        */
	std::unique_ptr<struct MultiOutputLogConfig> multiOutputConfig;  /*!< Multi-output configuration       */
	mutable std::mutex              multiOutputMutex;         /*!< Mutex for multi-output operations*/
	//------------------------------------------------------------------------------------------------
	// @} End of Private Members                                                                     +
	//------------------------------------------------------------------------------------------------
};


#endif // !INCLUDE_LIGHTLOGWRITEIMPL_HPP_

