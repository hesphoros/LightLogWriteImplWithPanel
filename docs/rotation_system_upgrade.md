# 日志轮转系统升级说明

## 概述

LightLogWriteImpl 的日志轮转系统已经从单体架构升级为企业级的模块化架构，提供了更强的可扩展性、更好的性能和更完善的错误处理机制。

## 新架构特性

### 🎯 核心改进
- **模块化设计**: 轮转逻辑完全分离，符合单一职责原则
- **策略模式**: 支持多种轮转策略，易于扩展
- **异步处理**: 支持非阻塞轮转操作，不影响日志写入性能
- **事务机制**: 原子操作保证数据一致性
- **状态机管理**: 12状态轮转流程控制
- **预检查系统**: 磁盘空间和权限验证
- **智能错误处理**: 分类错误处理和重试机制
- **精确时间计算**: 处理闰年、月末等边界情况

### 🔧 新增配置选项

```cpp
LogRotationConfig config;
// 基础配置
config.strategy = LogRotationStrategy::SizeAndTime;  // 轮转策略
config.maxFileSizeMB = 100;                         // 最大文件大小
config.timeInterval = TimeRotationInterval::Daily;  // 时间间隔
config.maxArchiveFiles = 10;                        // 最大归档文件数
config.enableCompression = true;                    // 启用压缩

// 新增高级配置
config.enableAsync = true;                          // 启用异步处理
config.asyncWorkerCount = 2;                        // 异步工作线程数  
config.enablePreCheck = true;                       // 启用预检查
config.enableTransaction = true;                    // 启用事务机制
config.enableStateMachine = true;                   // 启用状态机
config.maxRetryCount = 3;                          // 最大重试次数
config.retryDelay = std::chrono::milliseconds(1000); // 重试延迟
config.operationTimeout = std::chrono::milliseconds(30000); // 操作超时
config.diskSpaceThresholdMB = 1024;                // 磁盘空间阈值
```

## 新增接口方法

### 异步轮转
```cpp
// 异步强制轮转
std::future<bool> result = logger.ForceLogRotationAsync();
bool success = result.get(); // 等待完成
```

### 任务管理
```cpp
// 获取待处理任务数
size_t pending = logger.GetPendingRotationTasks();

// 取消待处理任务
size_t cancelled = logger.CancelPendingRotationTasks();
```

## 架构组件

### 1. ILogRotationManager (核心接口)
定义轮转管理器的统一接口，支持同步和异步操作。

### 2. IRotationStrategy (策略接口)
```cpp
- SizeBasedRotationStrategy: 基于文件大小的轮转
- TimeBasedRotationStrategy: 基于时间间隔的轮转  
- CompositeRotationStrategy: 组合多种策略
- ManualRotationStrategy: 手动触发轮转
```

### 3. AsyncRotationManager (异步管理器)
```cpp
- 任务队列管理
- 优先级处理
- 工作线程池
- 非阻塞操作
```

### 4. TransactionalRotation (事务机制)
```cpp
- 原子操作保证
- 回滚机制
- 操作日志
- 数据一致性
```

### 5. RotationStateMachine (状态机)
```cpp
12个状态的轮转流程:
Idle → PreCheck → BackupCreate → FileClose → FileMove → 
Compress → Archive → Cleanup → PostCheck → Complete → Error → Recovery
```

### 6. RotationPreChecker (预检查器)
```cpp
检查类型:
- 磁盘空间检查
- 文件权限检查  
- 目录访问检查
- 系统资源检查
- 配置有效性检查
```

### 7. RotationErrorHandler (错误处理器)
```cpp
错误分类:
- 磁盘空间不足
- 权限拒绝
- 文件锁定
- 网络错误
- 配置错误
- 系统资源不足
```

### 8. TimeCalculator (时间计算器)
```cpp
精确时间计算:
- 闰年处理
- 月末边界
- 时区转换
- 夏令时支持
```

## 使用示例

### 基础使用
```cpp
#include "LightLogWriteImpl.h"

// 创建日志实例
LightLogWrite_Impl logger;

// 配置轮转
LogRotationConfig config;
config.strategy = LogRotationStrategy::Size;
config.maxFileSizeMB = 50;
config.enableAsync = true;
logger.SetLogRotationConfig(config);

// 正常使用日志
logger.WriteLogInfo(L"测试消息");
```

### 高级使用
```cpp
// 设置回调监听轮转事件
logger.SubscribeToLogEvents([](const LogCallbackInfo& info) {
    if (info.level >= LogLevel::Info) {
        // 处理轮转通知
    }
});

// 手动控制轮转
if (someCondition) {
    auto future = logger.ForceLogRotationAsync();
    // 继续其他工作...
    bool success = future.get(); // 稍后获取结果
}

// 监控轮转状态
size_t pendingTasks = logger.GetPendingRotationTasks();
if (pendingTasks > 5) {
    logger.CancelPendingRotationTasks(); // 清理过多任务
}
```

## 性能优势

### 1. 异步处理
- 轮转操作不阻塞日志写入
- 后台任务队列处理
- 工作线程池并发处理

### 2. 智能调度
- 优先级队列管理
- 任务合并优化
- 资源使用最优化

### 3. 预检查机制
- 避免无效轮转尝试
- 提前发现问题
- 减少系统资源浪费

## 兼容性

### 向后兼容
- 保留所有原有公共接口
- 现有代码无需修改
- 配置格式保持兼容

### 升级建议
1. 逐步启用新功能
2. 监控轮转性能指标
3. 根据需要调整配置参数
4. 测试异步轮转行为

## 故障排除

### 常见问题
1. **轮转失败**: 检查磁盘空间和权限
2. **性能问题**: 调整异步工作线程数
3. **配置错误**: 使用预检查验证配置
4. **内存使用**: 监控任务队列大小

### 调试工具
```cpp
// 获取轮转统计
auto stats = rotationManager->GetStatistics();
std::cout << "成功率: " << stats.GetSuccessRate() << std::endl;

// 检查系统状态
auto config = logger.GetLogRotationConfig();
bool isHealthy = config.enablePreCheck;
```

## 总结

新的轮转系统提供了企业级的功能和性能，同时保持了简单易用的接口。通过模块化设计，系统具有良好的可扩展性和可维护性，能够满足各种复杂的日志轮转需求。