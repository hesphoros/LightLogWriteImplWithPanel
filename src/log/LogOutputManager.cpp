#include "../../include/log/LogOutputManager.h"
#include <algorithm>
#include <chrono>

LogOutputManager::LogOutputManager(const LogOutputManagerConfig& config)
    : m_config(config), m_shutdown(false), m_totalWrites(0), m_successfulWrites(0), m_failedWrites(0) {
    if (m_config.writeMode == OutputWriteMode::Async) {
        StartWorkerThreads();
    }
}

LogOutputManager::~LogOutputManager() {
    ShutdownAll();
}

bool LogOutputManager::AddOutput(LogOutputPtr output) {
    if (!output) return false;
    
    std::lock_guard<std::mutex> lock(m_outputsMutex);
    const std::wstring& name = output->GetOutputName();
    if (m_outputMap.find(name) != m_outputMap.end()) {
        return false;
    }
    
    m_outputs.push_back(output);
    m_outputMap[name] = output;
    return true;
}

bool LogOutputManager::RemoveOutput(const std::wstring& outputName) {
    std::lock_guard<std::mutex> lock(m_outputsMutex);
    auto mapIt = m_outputMap.find(outputName);
    if (mapIt == m_outputMap.end()) return false;
    
    auto vecIt = std::find(m_outputs.begin(), m_outputs.end(), mapIt->second);
    if (vecIt != m_outputs.end()) {
        (*vecIt)->Shutdown();
        m_outputs.erase(vecIt);
    }
    
    m_outputMap.erase(mapIt);
    return true;
}

LogOutputPtr LogOutputManager::GetOutput(const std::wstring& outputName) {
    std::lock_guard<std::mutex> lock(m_outputsMutex);
    auto it = m_outputMap.find(outputName);
    return (it != m_outputMap.end()) ? it->second : nullptr;
}

std::vector<LogOutputPtr> LogOutputManager::GetAllOutputs() const {
    std::lock_guard<std::mutex> lock(m_outputsMutex);
    return m_outputs;
}

void LogOutputManager::ClearOutputs() {
    std::lock_guard<std::mutex> lock(m_outputsMutex);
    for (auto& output : m_outputs) {
        if (output) output->Shutdown();
    }
    m_outputs.clear();
    m_outputMap.clear();
}

size_t LogOutputManager::GetOutputCount() const {
    std::lock_guard<std::mutex> lock(m_outputsMutex);
    return m_outputs.size();
}

std::vector<LogOutputResult> LogOutputManager::WriteLog(const LogCallbackInfo& logInfo) {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_totalWrites++;
    
    switch (m_config.writeMode) {
        case OutputWriteMode::Sequential:
            return WriteLogSequential(logInfo);
        case OutputWriteMode::Parallel:
            return WriteLogParallel(logInfo);
        default:
            return std::vector<LogOutputResult>();
    }
}

void LogOutputManager::FlushAll() {
    std::lock_guard<std::mutex> lock(m_outputsMutex);
    for (auto& output : m_outputs) {
        if (output && output->IsEnabled() && output->IsAvailable()) {
            try { output->Flush(); } catch (...) {}
        }
    }
}

bool LogOutputManager::IsAnyOutputAvailable() const {
    std::lock_guard<std::mutex> lock(m_outputsMutex);
    return std::any_of(m_outputs.begin(), m_outputs.end(),
        [](const LogOutputPtr& output) {
            return output && output->IsEnabled() && output->IsAvailable();
        });
}

void LogOutputManager::SetConfig(const LogOutputManagerConfig& config) {
    m_config = config;
}

LogOutputManagerConfig LogOutputManager::GetConfig() const {
    return m_config;
}

bool LogOutputManager::InitializeAll() {
    std::lock_guard<std::mutex> lock(m_outputsMutex);
    bool allSuccess = true;
    for (auto& output : m_outputs) {
        if (output && !output->Initialize()) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

void LogOutputManager::ShutdownAll() {
    std::lock_guard<std::mutex> lock(m_outputsMutex);
    for (auto& output : m_outputs) {
        if (output) output->Shutdown();
    }
}

// Private methods
void LogOutputManager::StartWorkerThreads() {
    // Simplified implementation for now
}

void LogOutputManager::StopWorkerThreads() {
    // Simplified implementation for now
}

void LogOutputManager::WorkerThreadFunction() {
    // Simplified implementation for now
}

std::vector<LogOutputResult> LogOutputManager::WriteLogSequential(const LogCallbackInfo& logInfo) {
    std::lock_guard<std::mutex> lock(m_outputsMutex);
    std::vector<LogOutputResult> results;
    results.reserve(m_outputs.size());
    
    bool anySuccess = false;
    for (auto& output : m_outputs) {
        if (!output || !output->IsEnabled()) {
            results.push_back(LogOutputResult::Unavailable);
            continue;
        }
        
        try {
            LogOutputResult result = output->WriteLog(logInfo);
            results.push_back(result);
            if (result == LogOutputResult::Success) {
                anySuccess = true;
            }
        } catch (...) {
            results.push_back(LogOutputResult::Failed);
        }
    }
    
    if (anySuccess) m_successfulWrites++;
    else m_failedWrites++;
    
    return results;
}

std::vector<LogOutputResult> LogOutputManager::WriteLogParallel(const LogCallbackInfo& logInfo) {
    // For now, fallback to sequential
    return WriteLogSequential(logInfo);
}

std::future<std::vector<LogOutputResult>> LogOutputManager::WriteLogAsync(const LogCallbackInfo& logInfo) {
    std::promise<std::vector<LogOutputResult>> promise;
    auto future = promise.get_future();
    promise.set_value(WriteLogSequential(logInfo));
    return future;
}

LogOutputManager::ManagerStats LogOutputManager::GetStatistics() const {
    std::lock_guard<std::mutex> statsLock(m_statsMutex);
    std::lock_guard<std::mutex> outputsLock(m_outputsMutex);
    
    ManagerStats stats;
    stats.totalWrites = m_totalWrites;
    stats.successfulWrites = m_successfulWrites;
    stats.failedWrites = m_failedWrites;
    stats.activeOutputs = std::count_if(m_outputs.begin(), m_outputs.end(),
        [](const LogOutputPtr& output) {
            return output && output->IsEnabled() && output->IsAvailable();
        });
    stats.queuedItems = 0;
    return stats;
}

void LogOutputManager::ResetStatistics() {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_totalWrites = 0;
    m_successfulWrites = 0;
    m_failedWrites = 0;
}
