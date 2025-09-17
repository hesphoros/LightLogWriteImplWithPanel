# 日志过滤器序列化功能使用指南

## 概述

本功能完善了LightLogWriteImpl项目中的过滤器序列化系统，解决了之前在`MultiOutputLogConfig.cpp`中标记为TODO的功能缺失问题。现在支持完整的过滤器创建、配置、序列化和反序列化功能。

## 功能特性

### ✅ 已实现的核心功能

1. **过滤器工厂模式** - 统一的过滤器创建和管理
2. **完整序列化支持** - JSON格式的配置持久化
3. **多种过滤器类型** - Level、Keyword、Regex、RateLimit、Thread
4. **配置文件集成** - 与现有JSON配置系统无缝集成
5. **类型安全** - 强类型检查和验证机制

### 🔧 支持的过滤器类型

| 过滤器类型 | 功能描述 | 配置参数 |
|-----------|----------|----------|
| Level | 按日志级别过滤 | minLevel, maxLevel, hasMaxLevel |
| Keyword | 按关键词过滤 | caseSensitive, includeKeywords, excludeKeywords |
| Regex | 正则表达式过滤 | pattern, isValid |
| RateLimit | 频率限制过滤 | maxPerSecond, maxBurst, availableTokens |
| Thread | 线程ID过滤 | useAllowList (运行时配置线程ID) |

## 使用方法

### 1. 基本过滤器创建

```cpp
#include "LogFilterFactory.h"

// 初始化过滤器工厂
LogFilterFactory::Initialize();

// 创建级别过滤器
auto levelFilter = LogFilterFactory::CreateFilter(L"Level");
if (levelFilter) {
    levelFilter->SetEnabled(true);
    levelFilter->SetPriority(10);
    
    // 转换为具体类型进行配置
    auto levelFilterImpl = dynamic_cast<LevelFilter*>(levelFilter.get());
    if (levelFilterImpl) {
        levelFilterImpl->SetMinLevel(LogLevel::Info);
        levelFilterImpl->SetMaxLevel(LogLevel::Error);
    }
}
```

### 2. 过滤器序列化

```cpp
// 序列化过滤器到JSON
nlohmann::json filterJson = LogFilterFactory::SerializeFilter(levelFilter.get());

// JSON输出示例
{
  "type": "Level",
  "enabled": true,
  "priority": 10,
  "description": "Filter logs by level range",
  "version": "1.0.0",
  "config": {
    "minLevel": "Info",
    "maxLevel": "Error",
    "hasMaxLevel": true
  }
}
```

### 3. 过滤器反序列化

```cpp
// 从JSON反序列化过滤器
auto deserializedFilter = LogFilterFactory::DeserializeFilter(filterJson);
if (deserializedFilter) {
    std::wcout << L"过滤器名称: " << deserializedFilter->GetFilterName() << L"\n";
    std::wcout << L"启用状态: " << deserializedFilter->IsEnabled() << L"\n";
}
```

### 4. 多输出配置中使用过滤器

```cpp
#include "MultiOutputLogConfig.h"

// 创建输出配置
OutputConfig output;
output.name = L"FilteredFileOutput";
output.type = L"File";
output.enabled = true;
output.useFilter = true;
output.filterType = L"Level";
output.filterConfig = L"{\"minLevel\":\"Info\",\"maxLevel\":\"Error\",\"hasMaxLevel\":true}";

// 添加到多输出配置
MultiOutputLogConfig multiConfig;
multiConfig.outputs.push_back(output);

// 序列化整个配置
nlohmann::json configJson = MultiOutputConfigSerializer::ToJson(multiConfig);
```

### 5. 配置文件操作

```cpp
// 保存配置到文件
bool saved = MultiOutputConfigSerializer::SaveToFile(config, L"config/filters.json");

// 从文件加载配置
MultiOutputLogConfig loadedConfig;
bool loaded = MultiOutputConfigSerializer::LoadFromFile(L"config/filters.json", loadedConfig);
```

## 配置文件格式

### 完整的配置文件示例

```json
{
  "configVersion": "1.0",
  "enabled": true,
  "globalMinLevel": "Info",
  "outputs": [
    {
      "name": "FileOutput",
      "type": "File",
      "enabled": true,
      "minLevel": "Debug",
      "config": "{\"filePath\":\"logs/app.log\"}",
      "useFilter": true,
      "filterType": "Level",
      "filterConfig": "{\"type\":\"Level\",\"enabled\":true,\"config\":{\"minLevel\":\"Debug\",\"maxLevel\":\"Fatal\",\"hasMaxLevel\":true}}"
    },
    {
      "name": "ConsoleOutput",
      "type": "Console", 
      "enabled": true,
      "minLevel": "Info",
      "config": "{\"useColors\":true}",
      "useFilter": true,
      "filterType": "Keyword",
      "filterConfig": "{\"type\":\"Keyword\",\"enabled\":true,\"config\":{\"caseSensitive\":false,\"includeKeywords\":[\"error\",\"warning\"],\"excludeKeywords\":[\"debug\"]}}"
    }
  ]
}
```

### 各类型过滤器配置示例

#### Level过滤器配置
```json
{
  "type": "Level",
  "enabled": true,
  "priority": 0,
  "config": {
    "minLevel": "Info",
    "maxLevel": "Error", 
    "hasMaxLevel": true
  }
}
```

#### Keyword过滤器配置  
```json
{
  "type": "Keyword",
  "enabled": true,
  "priority": 5,
  "config": {
    "caseSensitive": false,
    "includeKeywords": ["error", "warning", "exception"],
    "excludeKeywords": ["debug", "verbose"]
  }
}
```

#### Regex过滤器配置
```json
{
  "type": "Regex", 
  "enabled": true,
  "priority": 0,
  "config": {
    "pattern": "\\b(error|fail|exception)\\b",
    "isValid": true
  }
}
```

#### RateLimit过滤器配置
```json
{
  "type": "RateLimit",
  "enabled": true,
  "priority": 10,
  "config": {
    "maxPerSecond": 100,
    "maxBurst": 20,
    "availableTokens": 20
  }
}
```

## 示例程序

项目提供了完整的演示程序 `examples/filter_serialization_demo.cpp`，展示了：

1. **过滤器创建演示** - 各种类型过滤器的创建
2. **序列化演示** - 过滤器到JSON的转换
3. **多输出配置演示** - 完整配置的创建和序列化
4. **文件操作演示** - 配置文件的保存和加载

运行演示程序：
```bash
# 编译并运行
./examples/filter_serialization_demo
```

## 技术实现详情

### 架构设计

1. **工厂模式** - `LogFilterFactory`提供统一的过滤器创建接口
2. **策略模式** - 不同类型过滤器实现`ILogFilter`接口
3. **序列化器模式** - 每种过滤器类型有专门的序列化函数
4. **类型注册** - 动态注册和发现过滤器类型

### 关键类和接口

- `LogFilterFactory` - 过滤器工厂，管理创建和序列化
- `FilterTypeInfo` - 过滤器类型信息，包含创建和序列化函数
- `ILogFilter` - 过滤器基础接口
- `MultiOutputConfigSerializer` - 多输出配置序列化器

### 扩展性

要添加新的过滤器类型：

1. 实现`ILogFilter`接口
2. 创建类型信息`FilterTypeInfo`
3. 注册到工厂`LogFilterFactory::RegisterFilterType()`
4. 实现专门的序列化/反序列化函数

## 与现有系统集成

### 向后兼容

- 完全向后兼容现有配置格式
- 不影响已有的日志输出功能
- 可选择性启用过滤器功能

### 性能考虑

- 过滤器按优先级排序执行
- 支持快速拒绝机制`CanQuickReject()`
- 昂贵过滤器标记`IsExpensive()`
- 详细的性能统计信息

## 错误处理

- 完整的异常安全保证
- 配置验证和错误恢复
- 详细的错误信息和日志记录
- 优雅的降级处理

## 最佳实践

1. **优先级设置** - 按计算成本设置优先级，轻量级过滤器优先
2. **配置验证** - 使用`ValidateConfiguration()`验证配置
3. **性能监控** - 定期检查`GetStatistics()`了解过滤器性能
4. **资源管理** - 及时调用`Reset()`清理过滤器状态
5. **类型安全** - 使用工厂模式避免直接创建过滤器实例

## 故障排除

### 常见问题

1. **过滤器创建失败** - 检查类型名称是否正确注册
2. **序列化错误** - 验证JSON格式和必需字段
3. **配置加载失败** - 检查文件路径和权限
4. **性能问题** - 检查过滤器优先级和`IsExpensive()`标记

### 调试建议

- 启用详细日志记录
- 使用`GetStatistics()`监控过滤器性能
- 检查`IsEnabled()`状态
- 验证配置文件格式

## 总结

过滤器序列化功能现已完全实现，提供了：

✅ **完整的过滤器生命周期管理**  
✅ **灵活的配置和序列化机制**  
✅ **强类型安全和错误处理**  
✅ **良好的扩展性和性能**  
✅ **完整的文档和示例**  

这解决了项目中的关键TODO项，使日志系统的过滤功能更加完善和易用。