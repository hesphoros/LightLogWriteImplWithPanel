#pragma once

#include "ILogOutput.h"
#include "ILogFormatter.h"
#include "ILogFilter.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <future>

/**
 * @brief Output write mode enumeration
 * @details Defines how multiple outputs should be handled
 */
enum class OutputWriteMode {
    Sequential,  // Write to outputs one by one (synchronous)
    Parallel,    // Write to all outputs in parallel (asynchronous)
    Async        // Write to outputs asynchronously in background
};

/**
 * @brief Log output manager configuration
 */
struct LogOutputManagerConfig {
    OutputWriteMode writeMode = OutputWriteMode::Sequential;
    size_t asyncQueueSize = 1000;          // Max queue size for async mode
    size_t workerThreadCount = 2;          // Number of worker threads for async mode
    bool failFastOnError = false;          // Stop on first output error
    double writeTimeout = 5.0;             // Timeout in seconds for write operations
};

/**
 * @brief Manager for multiple log outputs
 * @details Coordinates writing to multiple log outputs, manages their lifecycle,
 *          and provides configuration management capabilities.
 */
class LogOutputManager {
private:
    // Output management
    std::vector<LogOutputPtr> m_outputs;
    std::unordered_map<std::wstring, LogOutputPtr> m_outputMap;
    mutable std::mutex m_outputsMutex;

    // Configuration
    LogOutputManagerConfig m_config;
    
    // Async processing
    std::queue<std::pair<LogCallbackInfo, std::promise<std::vector<LogOutputResult>>>> m_asyncQueue;
    std::vector<std::thread> m_workerThreads;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    std::atomic<bool> m_shutdown;

    // Statistics
    mutable std::mutex m_statsMutex;
    size_t m_totalWrites;
    size_t m_successfulWrites;
    size_t m_failedWrites;

public:
    /**
     * @brief Constructor
     * @param config Manager configuration
     */
    explicit LogOutputManager(const LogOutputManagerConfig& config = LogOutputManagerConfig{});

    /**
     * @brief Destructor
     */
    ~LogOutputManager();

    // Output management
    /**
     * @brief Add a log output
     * @param output Output to add
     * @return true if added successfully
     */
    bool AddOutput(LogOutputPtr output);

    /**
     * @brief Remove a log output by name
     * @param outputName Name of output to remove
     * @return true if removed successfully
     */
    bool RemoveOutput(const std::wstring& outputName);

    /**
     * @brief Get output by name
     * @param outputName Name of output to get
     * @return Output pointer or nullptr if not found
     */
    LogOutputPtr GetOutput(const std::wstring& outputName);

    /**
     * @brief Get all outputs
     * @return Vector of all outputs
     */
    std::vector<LogOutputPtr> GetAllOutputs() const;

    /**
     * @brief Clear all outputs
     */
    void ClearOutputs();

    /**
     * @brief Get number of outputs
     * @return Number of registered outputs
     */
    size_t GetOutputCount() const;

    // Logging operations
    /**
     * @brief Write log to all enabled outputs
     * @param logInfo Log information to write
     * @return Vector of results from each output
     */
    std::vector<LogOutputResult> WriteLog(const LogCallbackInfo& logInfo);

    /**
     * @brief Write log asynchronously
     * @param logInfo Log information to write
     * @return Future that will contain the results
     */
    std::future<std::vector<LogOutputResult>> WriteLogAsync(const LogCallbackInfo& logInfo);

    /**
     * @brief Flush all outputs
     */
    void FlushAll();

    /**
     * @brief Check if any output is available
     * @return true if at least one output is available
     */
    bool IsAnyOutputAvailable() const;

    // Configuration
    /**
     * @brief Set manager configuration
     * @param config New configuration
     */
    void SetConfig(const LogOutputManagerConfig& config);

    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    LogOutputManagerConfig GetConfig() const;

    // Statistics
    /**
     * @brief Get manager statistics
     */
    struct ManagerStats {
        size_t totalWrites;
        size_t successfulWrites;
        size_t failedWrites;
        size_t activeOutputs;
        size_t queuedItems;  // For async mode
    } GetStatistics() const;

    /**
     * @brief Reset statistics
     */
    void ResetStatistics();

    // Lifecycle management
    /**
     * @brief Initialize all outputs
     * @return true if all outputs initialized successfully
     */
    bool InitializeAll();

    /**
     * @brief Shutdown all outputs and cleanup
     */
    void ShutdownAll();

private:
    // Internal methods
    void StartWorkerThreads();
    void StopWorkerThreads();
    void WorkerThreadFunction();
    std::vector<LogOutputResult> WriteLogSequential(const LogCallbackInfo& logInfo);
    std::vector<LogOutputResult> WriteLogParallel(const LogCallbackInfo& logInfo);
};