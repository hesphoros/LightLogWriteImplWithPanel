# 调试宏系统使用指南

## 概述

LightLogWriteImplWithPanel项目现在包含了一个统一的调试宏系统，用于控制调试信息的输出。这个系统支持编译时和运行时的调试级别控制，并且可以针对不同模块进行精细化控制。

## 调试级别

系统定义了以下调试级别：

- **DEBUG_LEVEL_NONE (0)**: 无调试输出
- **DEBUG_LEVEL_ERROR (1)**: 仅错误信息
- **DEBUG_LEVEL_WARNING (2)**: 错误和警告信息
- **DEBUG_LEVEL_INFO (3)**: 错误、警告和信息
- **DEBUG_LEVEL_VERBOSE (4)**: 所有调试信息

## 编译时配置

通过在项目配置中定义以下宏来控制调试输出：

### 全局调试级别
```cpp
LIGHTLOG_DEBUG_LEVEL=3  // 设置全局调试级别为INFO
```

### 模块特定控制
```cpp
LIGHTLOG_DEBUG_MULTIOUTPUT=1  // 启用多输出模块调试
LIGHTLOG_DEBUG_CONSOLE=1      // 启用控制台模块调试  
LIGHTLOG_DEBUG_ROTATION=1     // 启用日志轮转模块调试
LIGHTLOG_DEBUG_COMPRESSION=1  // 启用压缩模块调试
```

## 当前项目配置

### Debug版本配置
- **LIGHTLOG_DEBUG_LEVEL=3**: 启用INFO级别及以下的所有调试信息
- **所有模块调试**: 启用所有模块的调试输出

### Release版本配置
- **LIGHTLOG_DEBUG_LEVEL=0**: 完全禁用调试输出，确保生产环境性能

## 使用方法

### 基础调试宏

```cpp
#include "../../include/log/DebugUtils.h"

// 基础调试输出
LIGHTLOG_DEBUG_ERROR(ModuleName, L"错误信息");
LIGHTLOG_DEBUG_WARNING(ModuleName, L"警告信息");
LIGHTLOG_DEBUG_INFO(ModuleName, L"信息");
LIGHTLOG_DEBUG_VERBOSE(ModuleName, L"详细信息");
```

### 模块特定宏

```cpp
// 多输出模块
LIGHTLOG_DEBUG_MULTIOUTPUT_INFO(L"多输出系统信息");
LIGHTLOG_DEBUG_MULTIOUTPUT_VERBOSE(L"多输出详细信息");
LIGHTLOG_DEBUG_MULTIOUTPUT_ERROR(L"多输出错误信息");

// 控制台模块
LIGHTLOG_DEBUG_CONSOLE_INFO(L"控制台信息");
LIGHTLOG_DEBUG_CONSOLE_VERBOSE(L"控制台详细信息");
LIGHTLOG_DEBUG_CONSOLE_ERROR(L"控制台错误信息");

// 日志轮转模块
LIGHTLOG_DEBUG_ROTATION_INFO(L"轮转信息");
LIGHTLOG_DEBUG_ROTATION_VERBOSE(L"轮转详细信息");
LIGHTLOG_DEBUG_ROTATION_ERROR(L"轮转错误信息");

// 压缩模块
LIGHTLOG_DEBUG_COMPRESSION_INFO(L"压缩信息");
LIGHTLOG_DEBUG_COMPRESSION_VERBOSE(L"压缩详细信息");
LIGHTLOG_DEBUG_COMPRESSION_ERROR(L"压缩错误信息");
```

### 流式调试宏

```cpp
// 用于复杂的调试信息组合
LIGHTLOG_DEBUG_STREAM(DEBUG_LEVEL_INFO, MultiOutput) 
    << L"处理项目: " << itemId << L", 状态: " << status 
    LIGHTLOG_DEBUG_STREAM_END(INFO, MultiOutput);
```

### 条件调试宏

```cpp
// 仅在特定条件下输出调试信息
LIGHTLOG_DEBUG_IF(condition, INFO, ModuleName, L"条件调试信息");
```

### 性能调试宏

```cpp
// 测量代码段执行时间
LIGHTLOG_DEBUG_PERFORMANCE_START(operationName);
// ... 要测量的代码 ...
LIGHTLOG_DEBUG_PERFORMANCE_END(operationName, ModuleName);
```

## 输出格式

调试信息的输出格式为：
```
[时间戳][级别][模块][线程ID] 消息内容
```

示例：
```
[14:30:15.123][INFO][MultiOutput][T:12345] Multi-output enabled - routing to output manager
```

## 性能优化

1. **编译时优化**: Release版本中所有调试宏都被编译器优化掉，不会产生任何运行时开销
2. **模块化控制**: 可以只启用需要调试的模块，减少不必要的输出
3. **级别控制**: 可以根据需要设置不同的调试级别

## 迁移说明

项目中原有的调试输出已经全部替换为新的宏系统：

### 替换前
```cpp
std::wcout << L"[DEBUG] Multi-output enabled" << std::endl;
```

### 替换后
```cpp
LIGHTLOG_DEBUG_MULTIOUTPUT_INFO(L"Multi-output enabled");
```

## 最佳实践

1. **开发阶段**: 使用DEBUG_LEVEL_INFO或DEBUG_LEVEL_VERBOSE获取详细信息
2. **测试阶段**: 使用DEBUG_LEVEL_WARNING减少输出噪音
3. **生产环境**: 使用DEBUG_LEVEL_NONE完全禁用调试输出
4. **问题诊断**: 临时启用特定模块的调试输出

## 扩展性

要添加新的调试模块：

1. 在`DebugUtils.h`中添加模块控制宏：
```cpp
#ifndef LIGHTLOG_DEBUG_NEWMODULE
    #define LIGHTLOG_DEBUG_NEWMODULE 1
#endif
```

2. 添加模块特定的调试宏：
```cpp
#if LIGHTLOG_DEBUG_NEWMODULE
    #define LIGHTLOG_DEBUG_NEWMODULE_INFO(message) LIGHTLOG_DEBUG_INFO(NewModule, message)
    // ... 其他级别
#else
    #define LIGHTLOG_DEBUG_NEWMODULE_INFO(message) do { } while(0)
    // ... 其他级别
#endif
```

3. 在项目配置中添加模块控制：
```cpp
LIGHTLOG_DEBUG_NEWMODULE=1
```

这样就可以实现对新模块的调试输出控制。