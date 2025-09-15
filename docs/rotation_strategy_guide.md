# 日志轮转策略使用指南

## 概述

LightLogWriteImpl 提供了灵活而强大的日志轮转策略系统，支持多种轮转触发条件和配置选项。本文档详细介绍了各种轮转策略的使用方法、配置参数和最佳实践。

## 目录

- [轮转策略类型](#轮转策略类型)
- [配置参数详解](#配置参数详解)
- [使用示例](#使用示例)
- [最佳实践](#最佳实践)
- [性能优化](#性能优化)
- [故障排除](#故障排除)

## 轮转策略类型

### 1. 基于文件大小的轮转 (Size-Based)

**适用场景：**
- 高频日志写入应用
- 需要严格控制单个日志文件大小
- 磁盘空间有限的环境

**配置示例：**
```cpp
LogRotationConfig config;
config.strategy = LogRotationStrategy::Size;
config.maxFileSizeMB = 50;              // 50MB 触发轮转
config.maxBackupFiles = 10;             // 保留10个备份文件
config.compressionEnabled = true;       // 启用压缩节省空间
```

**工作原理：**
- 监控当前日志文件大小
- 达到 `maxFileSizeMB` 阈值时自动触发轮转
- 生成带时间戳的备份文件
- 可选择压缩旧文件以节省空间

### 2. 基于时间间隔的轮转 (Time-Based)

**适用场景：**
- 需要定期归档日志
- 按时间段分析日志
- 合规性要求定期备份

**配置示例：**
```cpp
LogRotationConfig config;
config.strategy = LogRotationStrategy::Time;
config.timeInterval = TimeRotationInterval::Daily;  // 每天轮转
config.maxBackupFiles = 30;             // 保留30天备份
config.archiveOldFiles = true;          // 归档到指定目录
config.archivePath = L"logs/archive";   // 归档路径
```

**时间间隔选项：**
- `Hourly`: 每小时轮转
- `Daily`: 每天轮转  
- `Weekly`: 每周轮转
- `Monthly`: 每月轮转

### 3. 复合轮转策略 (Composite)

**适用场景：**
- 生产环境应用
- 既要控制文件大小又要定期归档
- 高可用性系统

**配置示例：**
```cpp
LogRotationConfig config;
config.strategy = LogRotationStrategy::SizeAndTime;
config.maxFileSizeMB = 100;             // 100MB 或
config.timeInterval = TimeRotationInterval::Daily;  // 每天
config.maxBackupFiles = 30;             // 保留30个备份
config.compressionEnabled = true;       // 启用压缩
config.asyncRotation = true;            // 异步轮转
```

**触发条件：**
- 文件大小达到阈值 **或** 时间间隔到达
- 优先处理大小限制，保证性能
- 时间轮转确保定期归档

### 4. 手动轮转 (Manual)

**适用场景：**
- 需要精确控制轮转时机
- 关键事件触发备份
- 维护窗口操作

**配置示例：**
```cpp
LogRotationConfig config;
config.strategy = LogRotationStrategy::None;  // 禁用自动轮转
config.enabled = true;
config.maxBackupFiles = 5;

// 手动触发轮转
rotationManager->ForceRotation(logFile, L"Critical event occurred");
```

## 配置参数详解

### 基础配置

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `enabled` | bool | false | 启用/禁用轮转功能 |
| `strategy` | LogRotationStrategy | None | 轮转策略类型 |
| `maxFileSizeMB` | size_t | 100 | 触发轮转的文件大小(MB) |
| `maxBackupFiles` | size_t | 5 | 最大备份文件数量 |
| `timeInterval` | TimeRotationInterval | Daily | 时间轮转间隔 |

### 压缩配置

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `compressionEnabled` | bool | false | 启用压缩 |
| `compressionLevel` | int | 6 | 压缩级别(1-9) |
| `compressionAlgorithm` | CompressionType | ZIP | 压缩算法 |
| `asyncCompression` | bool | true | 异步压缩 |

### 高级配置

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `asyncRotation` | bool | true | 异步轮转 |
| `archiveOldFiles` | bool | false | 归档旧文件 |
| `archivePath` | wstring | L"" | 归档目录路径 |
| `autoCleanup` | bool | false | 自动清理过期文件 |
| `cleanupOlderThanDays` | int | 30 | 清理天数阈值 |
| `diskSpaceThresholdMB` | size_t | 0 | 磁盘空间阈值 |
| `preserveFileAttributes` | bool | false | 保留文件属性 |
| `secureDelete` | bool | false | 安全删除文件 |

### 文件命名配置

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `rotatedFilePattern` | wstring | L"{basename}_{timestamp}.{extension}" | 轮转文件命名模式 |
| `timestampFormat` | wstring | L"%Y%m%d_%H%M%S" | 时间戳格式 |
| `useSequentialNumbers` | bool | false | 使用序号而非时间戳 |

## 使用示例

### 基础使用流程

```cpp
#include "log/LightLogWriteImpl.h"
#include "log/RotationManagerFactory.h"
#include "log/LogCompressor.h"

// 1. 创建配置
LogRotationConfig config;
config.strategy = LogRotationStrategy::Size;
config.enabled = true;
config.maxFileSizeMB = 50;
config.maxBackupFiles = 10;
config.compressionEnabled = true;

// 2. 创建压缩器和轮转管理器
auto compressor = std::make_shared<LogCompressor>();
auto rotationManager = RotationManagerFactory::CreateAsyncRotationManager(config, compressor);

// 3. 设置回调（可选）
rotationManager->SetRotationCallback([](const RotationResult& result) {
    if (result.success) {
        std::wcout << L"轮转成功: " << result.newFileName << L"\n";
    } else {
        std::wcout << L"轮转失败: " << result.errorMessage << L"\n";
    }
});

// 4. 启动轮转管理器
rotationManager->Start();

// 5. 在应用中使用
std::wstring logFile = L"app.log";
// 检查是否需要轮转
auto trigger = rotationManager->CheckRotationNeeded(logFile, currentFileSize);
if (trigger.sizeExceeded || trigger.timeReached) {
    // 自动处理轮转或手动触发
    rotationManager->ForceRotation(logFile, L"Rotation needed");
}

// 6. 关闭时停止管理器
rotationManager->Stop();
```

### 高级配置示例

```cpp
// 生产环境推荐配置
LogRotationConfig productionConfig;
productionConfig.strategy = LogRotationStrategy::SizeAndTime;
productionConfig.enabled = true;
productionConfig.maxFileSizeMB = 200;                    // 200MB
productionConfig.timeInterval = TimeRotationInterval::Daily;
productionConfig.maxBackupFiles = 30;                   // 保留30天
productionConfig.compressionEnabled = true;             // 启用压缩
productionConfig.compressionLevel = 6;                  // 平衡压缩比和速度
productionConfig.asyncRotation = true;                  // 异步轮转
productionConfig.asyncCompression = true;               // 异步压缩
productionConfig.archiveOldFiles = true;                // 归档
productionConfig.archivePath = L"logs/archive";         
productionConfig.autoCleanup = true;                    // 自动清理
productionConfig.cleanupOlderThanDays = 90;             // 90天后清理
productionConfig.diskSpaceThresholdMB = 1024;           // 1GB 磁盘阈值
productionConfig.preserveFileAttributes = true;         // 保留文件属性
productionConfig.rotatedFilePattern = L"{basename}_{timestamp}_{size}.{extension}";
productionConfig.timestampFormat = L"%Y%m%d_%H%M%S";
```

## 最佳实践

### 1. 策略选择指南

**选择基于大小的轮转：**
- 日志写入频率不规律
- 需要严格控制文件大小
- 存储空间有限

**选择基于时间的轮转：**
- 需要按时间段分析日志
- 合规性要求定期归档
- 日志写入频率相对稳定

**选择复合策略：**
- 生产环境应用
- 需要平衡性能和管理便利性
- 有足够的资源支持

### 2. 性能优化建议

**启用异步轮转：**
```cpp
config.asyncRotation = true;      // 减少对主线程的影响
config.asyncCompression = true;   // 压缩操作异步执行
```

**合理设置备份数量：**
```cpp
// 根据磁盘空间和访问需求平衡
config.maxBackupFiles = 20;       // 避免过多文件影响性能
```

**选择合适的压缩级别：**
```cpp
config.compressionLevel = 6;      // 平衡压缩比和CPU使用
// 1-3: 快速压缩，低CPU使用
// 4-6: 平衡模式（推荐）
// 7-9: 高压缩比，高CPU使用
```

### 3. 内存管理

**避免内存泄漏：**
```cpp
// 正确的资源管理
{
    auto rotationManager = RotationManagerFactory::CreateAsyncRotationManager(config, compressor);
    rotationManager->Start();
    
    // 使用轮转管理器...
    
    rotationManager->Stop();  // 确保正确停止
} // 自动释放资源
```

**监控内存使用：**
```cpp
// 定期检查统计信息
auto stats = rotationManager->GetStatistics();
if (stats.averageRotationTimeMs > 5000) {
    // 轮转时间过长，可能需要优化配置
    std::wcout << L"Warning: Rotation taking too long\n";
}
```

### 4. 错误处理

**设置错误回调：**
```cpp
rotationManager->SetRotationCallback([](const RotationResult& result) {
    if (!result.success) {
        // 记录错误，可能需要告警
        LogError(L"Rotation failed: " + result.errorMessage);
        
        // 可以尝试降级策略
        if (result.errorMessage.find(L"disk space") != std::wstring::npos) {
            // 磁盘空间不足，暂停轮转或清理旧文件
        }
    }
});
```

**磁盘空间监控：**
```cpp
config.diskSpaceThresholdMB = 500;  // 500MB 阈值
// 当磁盘空间不足时，自动停止轮转操作
```

## 性能优化

### 1. I/O 优化

**使用异步操作：**
```cpp
config.asyncRotation = true;        // 轮转操作不阻塞
config.asyncCompression = true;     // 压缩操作后台执行
```

**批量操作：**
- 避免频繁的小文件轮转
- 合理设置 `maxFileSizeMB` 参数
- 使用文件系统的原生操作

### 2. CPU 优化

**压缩级别选择：**
```cpp
// 根据 CPU 资源调整
config.compressionLevel = 1;        // CPU 受限环境
config.compressionLevel = 6;        // 平衡环境（推荐）
config.compressionLevel = 9;        // 存储优先环境
```

**多线程配置：**
```cpp
// 压缩器会自动使用多线程
auto compressor = std::make_shared<LogCompressor>();
// 内部使用 BS::thread_pool 进行并行处理
```

### 3. 内存优化

**控制并发轮转数量：**
```cpp
// 避免同时轮转过多文件
// 建议单个应用最多同时管理 10-20 个轮转管理器
```

**及时释放资源：**
```cpp
// 应用关闭时确保清理
rotationManager->Stop();
rotationManager.reset();
compressor.reset();
```

## 故障排除

### 常见问题

**1. 轮转失败**

**症状：** 轮转回调报告失败
**可能原因：**
- 磁盘空间不足
- 文件被其他进程占用
- 权限不足

**解决方案：**
```cpp
// 检查磁盘空间
config.diskSpaceThresholdMB = 200;

// 启用安全删除
config.secureDelete = true;

// 检查文件权限
config.preserveFileAttributes = false;
```

**2. 压缩失败**

**症状：** 文件轮转成功但未压缩
**可能原因：**
- 压缩库未正确链接
- 内存不足
- 文件格式不支持

**解决方案：**
```cpp
// 降低压缩级别
config.compressionLevel = 1;

// 检查压缩器状态
auto compressor = std::make_shared<LogCompressor>();
if (!compressor->IsAvailable()) {
    // 压缩器不可用，禁用压缩
    config.compressionEnabled = false;
}
```

**3. 性能问题**

**症状：** 轮转操作影响应用性能
**解决方案：**
```cpp
// 启用异步模式
config.asyncRotation = true;
config.asyncCompression = true;

// 调整轮转频率
config.maxFileSizeMB = 200;  // 增大文件大小阈值

// 减少备份文件数量
config.maxBackupFiles = 5;
```

### 调试信息

**启用详细日志：**
```cpp
rotationManager->SetRotationCallback([](const RotationResult& result) {
    std::wcout << L"Rotation details:\n";
    std::wcout << L"  Success: " << result.success << L"\n";
    std::wcout << L"  Original: " << result.originalFileName << L"\n";
    std::wcout << L"  New: " << result.newFileName << L"\n";
    std::wcout << L"  Time: " << result.rotationTimeMs << L" ms\n";
    std::wcout << L"  Trigger: " << result.trigger.reason << L"\n";
    
    if (result.compressionResult.compressed) {
        std::wcout << L"  Compression: " 
                  << result.compressionResult.originalSize << L" -> "
                  << result.compressionResult.compressedSize << L" bytes\n";
    }
});
```

**性能监控：**
```cpp
// 定期检查统计信息
auto stats = rotationManager->GetStatistics();
std::wcout << L"Rotation Statistics:\n";
std::wcout << L"  Total: " << stats.totalRotations << L"\n";
std::wcout << L"  Success Rate: " << (stats.successfulRotations * 100.0 / stats.totalRotations) << L"%\n";
std::wcout << L"  Average Time: " << stats.averageRotationTimeMs << L" ms\n";
std::wcout << L"  Space Saved: " << stats.totalSpaceSavedMB << L" MB\n";
```

## 总结

LightLogWriteImpl 的轮转策略系统提供了灵活而强大的日志管理功能。通过合理配置和使用最佳实践，可以实现高效、可靠的日志轮转和归档。

关键要点：
1. 根据应用场景选择合适的轮转策略
2. 启用异步操作以优化性能
3. 设置合理的配置参数
4. 实施适当的错误处理和监控
5. 定期检查和优化配置

更多示例代码请参考 `examples/rotation_strategy_examples.cpp` 文件。