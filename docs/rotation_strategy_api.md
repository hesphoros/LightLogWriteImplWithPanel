# IRotationStrategy API 参考文档

## 概述

`IRotationStrategy` 是 LightLogWriteImpl 轮转策略系统的核心接口，实现了策略模式来处理不同的日志轮转决策逻辑。本文档提供了完整的 API 参考和实现指南。

## 接口定义

### IRotationStrategy

```cpp
class IRotationStrategy {
public:
    virtual ~IRotationStrategy() = default;
    
    /**
     * @brief 判断是否需要执行轮转操作
     * @param context 轮转上下文信息
     * @return 轮转决策结果
     */
    virtual RotationDecision ShouldRotate(const RotationContext& context) = 0;
    
    /**
     * @brief 获取策略名称
     * @return 策略的可读名称
     */
    virtual std::wstring GetStrategyName() const = 0;
    
    /**
     * @brief 获取策略描述
     * @return 策略的详细描述
     */
    virtual std::wstring GetDescription() const = 0;
};
```

## 数据结构

### RotationContext

轮转上下文，包含进行轮转决策所需的所有信息：

```cpp
struct RotationContext {
    std::wstring filePath;                    // 日志文件路径
    size_t currentFileSize;                   // 当前文件大小（字节）
    std::chrono::system_clock::time_point lastRotationTime;  // 上次轮转时间
    std::chrono::system_clock::time_point currentTime;       // 当前时间
    LogRotationConfig config;                 // 轮转配置
    size_t availableDiskSpace;               // 可用磁盘空间（字节）
    bool isManualRequest;                    // 是否为手动请求
};
```

### RotationDecision

轮转决策结果：

```cpp
struct RotationDecision {
    bool shouldRotate;                       // 是否应该轮转
    std::wstring reason;                     // 轮转原因
    int priority;                           // 优先级（1-10，10最高）
    std::chrono::milliseconds estimatedTime; // 预估轮转时间
};
```

## 内置策略实现

### 1. SizeBasedRotationStrategy

基于文件大小的轮转策略。

**特点：**
- 监控文件大小
- 达到阈值时触发轮转
- 简单高效，适合高频写入场景

**实现示例：**
```cpp
class SizeBasedRotationStrategy : public IRotationStrategy {
public:
    RotationDecision ShouldRotate(const RotationContext& context) override {
        RotationDecision decision;
        decision.shouldRotate = false;
        decision.priority = 5;
        decision.estimatedTime = std::chrono::milliseconds(100);
        
        // 检查文件大小
        size_t thresholdBytes = context.config.maxFileSizeMB * 1024 * 1024;
        if (context.currentFileSize >= thresholdBytes) {
            decision.shouldRotate = true;
            decision.reason = L"File size exceeded " + 
                std::to_wstring(context.config.maxFileSizeMB) + L" MB";
            decision.priority = 8;
        }
        
        return decision;
    }
    
    std::wstring GetStrategyName() const override {
        return L"SizeBased";
    }
    
    std::wstring GetDescription() const override {
        return L"Rotates log files when they exceed a specified size threshold";
    }
};
```

### 2. TimeBasedRotationStrategy

基于时间间隔的轮转策略。

**特点：**
- 按固定时间间隔轮转
- 支持小时、日、周、月间隔
- 适合定期归档需求

**核心逻辑：**
```cpp
RotationDecision ShouldRotate(const RotationContext& context) override {
    RotationDecision decision;
    decision.shouldRotate = false;
    decision.priority = 6;
    
    auto timeSinceLastRotation = context.currentTime - context.lastRotationTime;
    auto requiredInterval = GetRequiredInterval(context.config.timeInterval);
    
    if (timeSinceLastRotation >= requiredInterval) {
        decision.shouldRotate = true;
        decision.reason = L"Time interval reached: " + 
            FormatInterval(context.config.timeInterval);
        decision.priority = 7;
    }
    
    return decision;
}

private:
std::chrono::duration<int64_t> GetRequiredInterval(TimeRotationInterval interval) {
    switch (interval) {
        case TimeRotationInterval::Hourly:
            return std::chrono::hours(1);
        case TimeRotationInterval::Daily:
            return std::chrono::hours(24);
        case TimeRotationInterval::Weekly:
            return std::chrono::hours(24 * 7);
        case TimeRotationInterval::Monthly:
            return std::chrono::hours(24 * 30);
        default:
            return std::chrono::hours(24);
    }
}
```

### 3. CompositeRotationStrategy

复合轮转策略，结合大小和时间条件。

**特点：**
- 支持多重触发条件
- 任一条件满足即触发轮转
- 适合生产环境使用

**实现要点：**
```cpp
RotationDecision ShouldRotate(const RotationContext& context) override {
    // 检查大小条件
    auto sizeDecision = sizeStrategy_.ShouldRotate(context);
    
    // 检查时间条件
    auto timeDecision = timeStrategy_.ShouldRotate(context);
    
    RotationDecision finalDecision;
    
    if (sizeDecision.shouldRotate || timeDecision.shouldRotate) {
        finalDecision.shouldRotate = true;
        
        // 选择优先级更高的原因
        if (sizeDecision.priority >= timeDecision.priority) {
            finalDecision.reason = sizeDecision.reason;
            finalDecision.priority = sizeDecision.priority;
        } else {
            finalDecision.reason = timeDecision.reason;
            finalDecision.priority = timeDecision.priority;
        }
        
        // 组合原因
        if (sizeDecision.shouldRotate && timeDecision.shouldRotate) {
            finalDecision.reason = L"Size and time conditions met";
            finalDecision.priority = 9;
        }
    }
    
    return finalDecision;
}
```

### 4. ManualRotationStrategy

手动轮转策略。

**特点：**
- 仅响应手动触发
- 不执行自动检查
- 适合精确控制场景

## 自定义策略实现

### 实现步骤

1. **继承 IRotationStrategy 接口**
2. **实现必要的方法**
3. **注册到轮转管理器**

### 示例：磁盘空间策略

```cpp
class DiskSpaceRotationStrategy : public IRotationStrategy {
private:
    double thresholdPercentage_ = 0.1;  // 10% 可用空间阈值
    
public:
    DiskSpaceRotationStrategy(double threshold = 0.1) 
        : thresholdPercentage_(threshold) {}
    
    RotationDecision ShouldRotate(const RotationContext& context) override {
        RotationDecision decision;
        decision.shouldRotate = false;
        decision.priority = 4;
        decision.estimatedTime = std::chrono::milliseconds(50);
        
        // 获取磁盘总空间
        auto totalSpace = GetTotalDiskSpace(context.filePath);
        if (totalSpace == 0) {
            return decision;  // 无法获取磁盘信息
        }
        
        // 计算可用空间百分比
        double availablePercentage = static_cast<double>(context.availableDiskSpace) / totalSpace;
        
        if (availablePercentage < thresholdPercentage_) {
            decision.shouldRotate = true;
            decision.reason = L"Low disk space: " + 
                std::to_wstring(availablePercentage * 100) + L"% available";
            decision.priority = 9;  // 高优先级
        }
        
        return decision;
    }
    
    std::wstring GetStrategyName() const override {
        return L"DiskSpaceBased";
    }
    
    std::wstring GetDescription() const override {
        return L"Rotates when available disk space falls below threshold";
    }
    
private:
    size_t GetTotalDiskSpace(const std::wstring& filePath) {
        // 实现获取磁盘总空间的逻辑
        // Windows: GetDiskFreeSpaceExW
        // Linux: statvfs
        return 0;  // 简化示例
    }
};
```

### 示例：负载均衡策略

```cpp
class LoadBalancedRotationStrategy : public IRotationStrategy {
private:
    std::atomic<int> rotationCounter_{0};
    int maxRotationsPerMinute_ = 5;
    std::chrono::steady_clock::time_point lastMinute_;
    
public:
    RotationDecision ShouldRotate(const RotationContext& context) override {
        RotationDecision decision;
        decision.shouldRotate = false;
        decision.priority = 3;
        
        auto now = std::chrono::steady_clock::now();
        
        // 重置计数器（每分钟）
        if (now - lastMinute_ >= std::chrono::minutes(1)) {
            rotationCounter_ = 0;
            lastMinute_ = now;
        }
        
        // 检查是否超过速率限制
        if (rotationCounter_ >= maxRotationsPerMinute_) {
            decision.reason = L"Rate limit exceeded";
            return decision;
        }
        
        // 基于文件大小进行基础检查
        size_t thresholdBytes = context.config.maxFileSizeMB * 1024 * 1024;
        if (context.currentFileSize >= thresholdBytes) {
            decision.shouldRotate = true;
            decision.reason = L"Size threshold with rate limiting";
            decision.priority = 6;
            
            // 增加计数器
            rotationCounter_++;
        }
        
        return decision;
    }
    
    std::wstring GetStrategyName() const override {
        return L"LoadBalanced";
    }
    
    std::wstring GetDescription() const override {
        return L"Rate-limited size-based rotation to prevent system overload";
    }
};
```

## 策略注册和使用

### 注册自定义策略

```cpp
// 创建自定义策略实例
auto customStrategy = std::make_unique<DiskSpaceRotationStrategy>(0.15);  // 15% 阈值

// 创建轮转管理器
LogRotationConfig config;
config.strategy = LogRotationStrategy::None;  // 禁用内置策略

auto compressor = std::make_shared<LogCompressor>();
auto manager = RotationManagerFactory::CreateAsyncRotationManager(config, compressor);

// 在实际实现中，需要扩展管理器接口来支持自定义策略
// manager->SetCustomStrategy(std::move(customStrategy));
```

### 策略组合使用

```cpp
class MultiStrategyRotationManager {
private:
    std::vector<std::unique_ptr<IRotationStrategy>> strategies_;
    
public:
    void AddStrategy(std::unique_ptr<IRotationStrategy> strategy) {
        strategies_.push_back(std::move(strategy));
    }
    
    RotationDecision CheckAllStrategies(const RotationContext& context) {
        RotationDecision finalDecision;
        finalDecision.shouldRotate = false;
        finalDecision.priority = 0;
        
        for (auto& strategy : strategies_) {
            auto decision = strategy->ShouldRotate(context);
            
            if (decision.shouldRotate && decision.priority > finalDecision.priority) {
                finalDecision = decision;
            }
        }
        
        return finalDecision;
    }
};
```

## 最佳实践

### 1. 策略性能优化

```cpp
class OptimizedSizeStrategy : public IRotationStrategy {
private:
    mutable std::chrono::steady_clock::time_point lastCheck_;
    mutable bool lastResult_ = false;
    static constexpr auto CHECK_INTERVAL = std::chrono::seconds(10);
    
public:
    RotationDecision ShouldRotate(const RotationContext& context) override {
        auto now = std::chrono::steady_clock::now();
        
        // 缓存结果，避免频繁检查
        if (now - lastCheck_ < CHECK_INTERVAL && !lastResult_) {
            RotationDecision decision;
            decision.shouldRotate = false;
            decision.reason = L"Recently checked, no rotation needed";
            return decision;
        }
        
        lastCheck_ = now;
        
        // 执行实际检查
        auto decision = DoSizeCheck(context);
        lastResult_ = decision.shouldRotate;
        
        return decision;
    }
    
private:
    RotationDecision DoSizeCheck(const RotationContext& context) {
        // 实际的大小检查逻辑
        // ...
    }
};
```

### 2. 错误处理

```cpp
class SafeRotationStrategy : public IRotationStrategy {
public:
    RotationDecision ShouldRotate(const RotationContext& context) override {
        try {
            return DoRotationCheck(context);
        } catch (const std::exception& e) {
            // 记录错误但不抛出异常
            LogError(L"Strategy error: " + std::wstring(e.what()));
            
            // 返回安全的默认决策
            RotationDecision safeDecision;
            safeDecision.shouldRotate = false;
            safeDecision.reason = L"Strategy error, skipping rotation";
            safeDecision.priority = 0;
            
            return safeDecision;
        }
    }
    
private:
    RotationDecision DoRotationCheck(const RotationContext& context) {
        // 可能抛出异常的检查逻辑
        // ...
    }
    
    void LogError(const std::wstring& message) {
        // 错误日志记录
        // ...
    }
};
```

### 3. 策略配置

```cpp
class ConfigurableStrategy : public IRotationStrategy {
private:
    struct StrategyConfig {
        size_t sizeThresholdMB = 50;
        std::chrono::minutes timeThreshold{60};
        double diskSpaceThreshold = 0.1;
        bool enableSizeCheck = true;
        bool enableTimeCheck = true;
        bool enableDiskCheck = false;
    };
    
    StrategyConfig config_;
    
public:
    ConfigurableStrategy(const StrategyConfig& config = {}) 
        : config_(config) {}
    
    void UpdateConfig(const StrategyConfig& newConfig) {
        config_ = newConfig;
    }
    
    RotationDecision ShouldRotate(const RotationContext& context) override {
        RotationDecision decision;
        decision.shouldRotate = false;
        decision.priority = 1;
        
        // 根据配置执行不同检查
        if (config_.enableSizeCheck) {
            auto sizeDecision = CheckSize(context);
            if (sizeDecision.shouldRotate) {
                return sizeDecision;
            }
        }
        
        if (config_.enableTimeCheck) {
            auto timeDecision = CheckTime(context);
            if (timeDecision.shouldRotate) {
                return timeDecision;
            }
        }
        
        if (config_.enableDiskCheck) {
            auto diskDecision = CheckDiskSpace(context);
            if (diskDecision.shouldRotate) {
                return diskDecision;
            }
        }
        
        return decision;
    }
    
private:
    RotationDecision CheckSize(const RotationContext& context) { /* ... */ }
    RotationDecision CheckTime(const RotationContext& context) { /* ... */ }
    RotationDecision CheckDiskSpace(const RotationContext& context) { /* ... */ }
};
```

