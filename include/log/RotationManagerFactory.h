#pragma once

#include <memory>
#include "ILogRotationManager.h"

// Forward declarations
class ILogCompressor;

/**
 * @brief Factory class for creating rotation managers
 * @details Provides static methods to create different types of rotation managers
 */
class RotationManagerFactory {
public:
    /**
     * @brief Create default rotation manager with async support
     * @param config The rotation configuration
     * @param compressor Optional compressor for ZIP compression
     * @return Unique pointer to the rotation manager
     */
    static std::unique_ptr<ILogRotationManager> CreateAsyncRotationManager(
        const LogRotationConfig& config = LogRotationConfig{},
        std::shared_ptr<ILogCompressor> compressor = nullptr
    );

    /**
     * @brief Create synchronous rotation manager
     * @param config The rotation configuration
     * @param compressor Optional compressor for ZIP compression
     * @return Unique pointer to the rotation manager
     */
    static std::unique_ptr<ILogRotationManager> CreateSyncRotationManager(
        const LogRotationConfig& config = LogRotationConfig{},
        std::shared_ptr<ILogCompressor> compressor = nullptr
    );

private:
    RotationManagerFactory() = delete;
    ~RotationManagerFactory() = delete;
    RotationManagerFactory(const RotationManagerFactory&) = delete;
    RotationManagerFactory& operator=(const RotationManagerFactory&) = delete;
};

