#include "../../include/log/RotationManagerFactory.h"
#include "../../include/log/ILogRotationManager.h"
#include "../../include/log/IRotationStrategy.h"
#include "../../include/log/ILogCompressor.h"
#include <memory>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <chrono>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/stat.h>
#endif

/**
 * @brief Simple rotation manager implementation for basic functionality
 * @details Provides a minimal implementation of ILogRotationManager for testing
 */
class SimpleRotationManager : public ILogRotationManager {
public:
    explicit SimpleRotationManager(const LogRotationConfig& config, 
                                  std::shared_ptr<ILogCompressor> compressor = nullptr) 
        : config_(config), isRunning_(false), compressor_(compressor) {}

    void SetConfig(const LogRotationConfig& config) override {
        config_ = config;
    }

    LogRotationConfig GetConfig() const override {
        return config_;
    }

    RotationTrigger CheckRotationNeeded(const std::wstring& currentFileName, 
                                       size_t fileSize) const override {
        RotationTrigger trigger;
        
        if (config_.strategy == LogRotationStrategy::None) {
            return trigger;
        }
        
        // Simple size-based check
        if (config_.strategy == LogRotationStrategy::Size || 
            config_.strategy == LogRotationStrategy::SizeAndTime) {
            if (fileSize >= config_.maxFileSizeMB * 1024 * 1024) {
                trigger.sizeExceeded = true;
                trigger.currentFileSize = fileSize;
                trigger.reason = L"File size limit reached";
            }
        }
        
        return trigger;
    }

    RotationResult PerformRotation(const std::wstring& currentFileName,
                                  const RotationTrigger& trigger) override {
        std::cout << "[RotationManager] PerformRotation started for file: " 
                  << std::string(currentFileName.begin(), currentFileName.end()) << "\n";
        
#ifdef _WIN32
        std::cout << "[RotationManager] Process ID: " << GetCurrentProcessId() 
                  << ", Thread ID: " << GetCurrentThreadId() << "\n";
#endif
        
        RotationResult result;
        result.success = false;
        result.oldFileName = currentFileName;
        result.rotationTime = std::chrono::system_clock::now();
        auto startTime = std::chrono::high_resolution_clock::now();
        
        try {
            // Ensure archive directory exists
            if (!config_.archiveDirectory.empty()) {
                // Create archive directory using Windows API or cross-platform method
#ifdef _WIN32
                CreateDirectoryW(config_.archiveDirectory.c_str(), NULL);
#else
                // For Linux/Unix, use mkdir command or create directory manually
                std::string archiveDirStr(config_.archiveDirectory.begin(), config_.archiveDirectory.end());
                mkdir(archiveDirStr.c_str(), 0755);
#endif
                
                // Generate archive file name with timestamp
                auto now = std::chrono::system_clock::now();
                auto timeT = std::chrono::system_clock::to_time_t(now);
                std::tm tmInfo;
#ifdef _WIN32
                localtime_s(&tmInfo, &timeT);
#else
                localtime_r(&timeT, &tmInfo);
#endif
                
                std::wostringstream oss;
                // Extract base name from current file name (simple approach)
                std::wstring baseName;
                size_t lastSlash = currentFileName.find_last_of(L"\\/");
                if (lastSlash != std::wstring::npos) {
                    baseName = currentFileName.substr(lastSlash + 1);
                } else {
                    baseName = currentFileName;
                }
                
                // Remove extension
                size_t lastDot = baseName.find_last_of(L'.');
                if (lastDot != std::wstring::npos) {
                    baseName = baseName.substr(0, lastDot);
                }
                
                oss << baseName << L"_" << std::put_time(&tmInfo, L"%Y%m%d_%H%M%S");
                
                if (config_.enableCompression && compressor_) {
                    oss << L".zip";
                } else {
                    oss << L".log";
                }
                
                std::wstring archiveFileName = oss.str();
                std::wstring fullArchivePath = config_.archiveDirectory;
                if (!fullArchivePath.empty() && fullArchivePath.back() != L'\\' && fullArchivePath.back() != L'/') {
                    fullArchivePath += L"/";
                }
                fullArchivePath += archiveFileName;
                
                // Check if source file exists and has content
                // Convert wstring to string for file operations (Windows specific)
                int len = WideCharToMultiByte(CP_UTF8, 0, currentFileName.c_str(), -1, NULL, 0, NULL, NULL);
                std::string currentFileNameStr(len, 0);
                WideCharToMultiByte(CP_UTF8, 0, currentFileName.c_str(), -1, &currentFileNameStr[0], len, NULL, NULL);
                currentFileNameStr.resize(len - 1); // Remove null terminator
                
                std::cout << "[RotationManager] Opening source file for size check: " << currentFileNameStr << "\n";
                
                std::ifstream source(currentFileNameStr, std::ios::binary | std::ios::ate);
                if (source.is_open()) {
                    auto fileSize = source.tellg();
                    source.seekg(0, std::ios::beg);
                    
                    // Only create archive file if source has content
                    if (fileSize > 0) {
                        if (config_.enableCompression && compressor_) {
                            // Use LogCompressor for real ZIP compression
                            std::cout << "[RotationManager] Preparing for compression...\n";
                            source.close(); // Close source before compression
                            
                            std::cout << "[RotationManager] Calling CompressFile...\n";
                            if (compressor_->CompressFile(currentFileName, fullArchivePath)) {
                                std::cout << "[RotationManager] Compression completed successfully\n";
                                result.archiveFileName = fullArchivePath;
                            } else {
                                std::cout << "[RotationManager] Compression failed, falling back to copy\n";
                                // Compression failed, fall back to simple copy
                                std::ifstream fallbackSource(currentFileNameStr, std::ios::binary);
                                if (fallbackSource.is_open()) {
                                    // Change extension back to .log for uncompressed fallback
                                    std::wstring fallbackPath = fullArchivePath;
                                    size_t lastDot = fallbackPath.find_last_of(L'.');
                                    if (lastDot != std::wstring::npos) {
                                        fallbackPath = fallbackPath.substr(0, lastDot) + L".log";
                                    }
                                    
                                    int destLen = WideCharToMultiByte(CP_UTF8, 0, fallbackPath.c_str(), -1, NULL, 0, NULL, NULL);
                                    std::string destPathStr(destLen, 0);
                                    WideCharToMultiByte(CP_UTF8, 0, fallbackPath.c_str(), -1, &destPathStr[0], destLen, NULL, NULL);
                                    destPathStr.resize(destLen - 1);
                                    
                                    std::ofstream dest(destPathStr, std::ios::binary);
                                    if (dest.is_open()) {
                                        dest << fallbackSource.rdbuf();
                                        dest.close();
                                        result.archiveFileName = fallbackPath;
                                    }
                                    fallbackSource.close();
                                }
                            }
                        } else {
                            // Simple copy for uncompressed archives
                            int destLen = WideCharToMultiByte(CP_UTF8, 0, fullArchivePath.c_str(), -1, NULL, 0, NULL, NULL);
                            std::string destPathStr(destLen, 0);
                            WideCharToMultiByte(CP_UTF8, 0, fullArchivePath.c_str(), -1, &destPathStr[0], destLen, NULL, NULL);
                            destPathStr.resize(destLen - 1);
                            
                            std::ofstream dest(destPathStr, std::ios::binary);
                            if (dest.is_open()) {
                                dest << source.rdbuf();
                                dest.close();
                                result.archiveFileName = fullArchivePath;
                            }
                        }
                    } else {
                        // Source file is empty, don't create archive
                        result.archiveFileName = L""; // No archive created for empty file
                    }
                    
                    source.close();
                    
                    // Clear the current file (truncate it) regardless of content
                    std::ofstream clearFile(currentFileNameStr, std::ios::trunc);
                    clearFile.close();
                }
                
                result.archiveFileName = fullArchivePath;
            }
            
            result.success = true;
            result.newFileName = currentFileName; // Keep same active file name
            result.errorMessage = "Rotation completed successfully";
            
        } catch (const std::exception& e) {
            result.success = false;
            result.errorMessage = std::string("Rotation failed: ") + e.what();
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // Update statistics
        statistics_.totalRotations++;
        if (result.success) {
            statistics_.successfulRotations++;
        } else {
            statistics_.failedRotations++;
        }
        
        return result;
    }

    RotationResult ForceRotation(const std::wstring& currentFileName,
                                const std::wstring& reason = L"Manual rotation") override {
        RotationTrigger trigger;
        trigger.manualRequested = true;
        trigger.reason = reason;
        return PerformRotation(currentFileName, trigger);
    }

    std::future<RotationResult> PerformRotationAsync(const std::wstring& currentFileName,
                                                     const RotationTrigger& trigger) override {
        // 创建真正的异步任务，避免阻塞
        return std::async(std::launch::async, [this, currentFileName, trigger]() {
            std::cout << "[RotationManager] Starting async rotation...\n";
            try {
                auto result = PerformRotation(currentFileName, trigger);
                std::cout << "[RotationManager] Async rotation completed with result: " 
                         << (result.success ? "SUCCESS" : "FAILED") << "\n";
                return result;
            }
            catch (const std::exception& e) {
                std::cout << "[RotationManager] Exception in async rotation: " << e.what() << "\n";
                RotationResult result;
                result.success = false;
                result.errorMessage = std::string("Async rotation failed: ") + e.what();
                return result;
            }
            catch (...) {
                std::cout << "[RotationManager] Unknown exception in async rotation\n";
                RotationResult result;
                result.success = false;
                result.errorMessage = "Async rotation failed: Unknown error";
                return result;
            }
        });
    }

    void SetRotationCallback(RotationCallback callback) override {
        callback_ = callback;
    }

    RotationStatistics GetStatistics() const override {
        return statistics_;
    }

    void ResetStatistics() override {
        statistics_ = RotationStatistics{};
    }

    size_t CleanupOldArchives() override {
        return 0; // No cleanup in simple implementation
    }

    bool GetNextRotationTime(std::chrono::system_clock::time_point& nextTime) const override {
        return false; // No time-based rotation in simple implementation
    }

    void Start() override {
        isRunning_ = true;
    }

    void Stop() override {
        isRunning_ = false;
    }

    bool IsRunning() const override {
        return isRunning_;
    }

    size_t GetPendingTaskCount() const override {
        return 0;
    }

    size_t GetActiveTaskCount() const override {
        return 0;
    }

    size_t CancelPendingTasks() override {
        return 0;
    }

    bool WaitForAllTasks(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) override {
        return true;
    }

private:
    LogRotationConfig config_;
    RotationStatistics statistics_;
    RotationCallback callback_;
    bool isRunning_;
    std::shared_ptr<ILogCompressor> compressor_;
};

std::unique_ptr<ILogRotationManager> RotationManagerFactory::CreateAsyncRotationManager(
    const LogRotationConfig& config, std::shared_ptr<ILogCompressor> compressor)
{
    // For now, return simple implementation
    // In a complete implementation, this would return AsyncRotationManager
    return std::make_unique<SimpleRotationManager>(config, compressor);
}

std::unique_ptr<ILogRotationManager> RotationManagerFactory::CreateSyncRotationManager(
    const LogRotationConfig& config, std::shared_ptr<ILogCompressor> compressor)
{
    return std::make_unique<SimpleRotationManager>(config, compressor);
}