# LightLog - Modern C++17 Logging Library

<div align="center">

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![CMake](https://img.shields.io/badge/CMake-3.16+-green.svg)
![License](https://img.shields.io/badge/License-GPL%20v3-green.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)
![Build](https://img.shields.io/badge/Build-Passing-brightgreen.svg)

**现代化企业级C++日志库**

*功能完整 • 性能卓越 • 现代CMake • 易于集成*

</div>

---

## 📋 项目概述

LightLog (LightLogWriteImplWithPanel) 是一个现代化的企业级C++17日志库，提供全面的日志记录、轮转、压缩、过滤和多输出功能。采用现代CMake构建系统，支持多种集成方式，专为高并发、大规模应用场景设计。

### ✨ 核心特性

- 🚀 **高性能**: 异步队列处理、多线程优化、支持批处理操作
- 🔄 **智能轮转**: 支持基于大小、时间、复合等多种轮转策略
- 📦 **自动压缩**: 内置ZIP/GZIP/LZ4/ZSTD压缩算法，节省存储空间
- 🔍 **强大过滤**: 级别、关键词、正则表达式、频率限制等多种过滤方式
- 🎯 **多输出支持**: 控制台、文件等多种输出目标，支持同时输出
- 🔒 **线程安全**: 完整的多线程支持和线程安全保证
- 🌐 **跨平台**: 支持Windows和Linux平台
- 🏗️ **现代CMake**: 支持FetchContent、add_subdirectory、find_package三种集成方式
- ⚙️ **易配置**: 灵活的JSON配置系统和丰富的API
- 📦 **依赖管理**: 通过FetchContent自动管理外部依赖

---

## 🚀 快速开始

### 基础使用

```cpp
#include "log/LightLogWriteImpl.h"

int main() {
    // 创建日志实例
    auto logger = std::make_shared<LightLogWrite_Impl>();
    
    // 设置日志文件（目录和基础名称）
    logger->SetLastingsLogs(L"logs", L"app");
    
    // 写入不同级别的日志
    logger->WriteLogInfo(L"应用程序启动");
    logger->WriteLogWarning(L"这是一个警告消息");
    logger->WriteLogError(L"发生了一个错误");
    
    // 或使用通用接口
    logger->WriteLogContent(LogLevel::Info, L"信息日志");
    logger->WriteLogContent(LogLevel::Debug, L"调试日志");
    
    return 0;
}
```

### 高级配置示例

```cpp
#include "log/LightLogWriteImpl.h"
#include "log/LogCompressor.h"
#include "log/ILogRotationManager.h"

int main() {
    // 1. 创建压缩器（可选）
    LogCompressorConfig compressorConfig;
    compressorConfig.algorithm = CompressionAlgorithm::ZIP;
    compressorConfig.compressionLevel = 6;              // 压缩级别 1-9
    compressorConfig.workerThreadCount = 4;             // 工作线程数
    compressorConfig.deleteSourceAfterSuccess = true;   // 压缩后删除源文件
    auto compressor = std::make_shared<LogCompressor>(compressorConfig);
    compressor->Start();  // 启动压缩服务
    
    // 2. 创建日志记录器
    auto logger = std::make_shared<LightLogWrite_Impl>(
        10000,                              // 日志队列大小
        LogQueueOverflowStrategy::Block,    // 队列满时的策略（Block或DropOldest）
        1000,                               // 批处理大小
        compressor                          // 压缩器（可选，传nullptr禁用压缩）
    );
    
    // 3. 设置日志文件路径
    logger->SetLastingsLogs(L"logs", L"myapp");
    
    // 4. 配置日志轮转
    LogRotationConfig rotationConfig;
    rotationConfig.strategy = LogRotationStrategy::SizeAndTime;  // 复合策略
    rotationConfig.maxFileSizeMB = 100;                         // 100MB触发轮转
    rotationConfig.timeInterval = TimeRotationInterval::Daily;  // 每天轮转
    rotationConfig.maxArchiveFiles = 30;                        // 保留30个归档
    rotationConfig.enableCompression = true;                    // 启用压缩
    rotationConfig.enableAsync = true;                          // 异步轮转
    rotationConfig.archiveDirectory = L"logs/archive";          // 归档目录
    logger->SetLogRotationConfig(rotationConfig);
    
    // 5. 设置日志级别过滤
    logger->SetMinLogLevel(LogLevel::Info);  // 只记录Info及以上级别
    
    // 6. 使用日志系统
    logger->WriteLogInfo(L"系统配置完成");
    logger->WriteLogDebug(L"这条不会被记录（低于Info级别）");
    
    return 0;
}
```

---

## 📦 安装说明

### 环境要求

- **C++标准**: C++17或更高版本
- **CMake**: 3.16或更高版本
- **编译器**:
  - Windows: Visual Studio 2019或更高版本
  - Linux: GCC 7.0+或Clang 6.0+
- **外部依赖**: 完全自动管理，无需手动安装
  - UniConv (通过FetchContent自动获取)
  - nlohmann/json (已包含)
  - BS::thread_pool (已包含)
  - miniz (已包含)

### 方式一：作为独立项目构建

```bash
# 克隆仓库
git clone https://github.com/hesphoros/LightLogWriteImplWithPanel.git
cd LightLogWriteImplWithPanel

# CMake 构建
mkdir build && cd build
cmake .. -DLIGHTLOG_BUILD_EXAMPLES=ON -DLIGHTLOG_BUILD_TESTS=ON
cmake --build . --config Release

# 运行主演示程序
./bin/Release/lightlog_demo

# Windows用户
.\bin\Release\lightlog_demo.exe
```

### 方式二：作为子项目集成

```cmake
# 在你的CMakeLists.txt中添加子目录
add_subdirectory(external/LightLog)

# 链接到你的目标
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE LightLog::lightlog)
```

### 方式三：使用FetchContent集成（推荐）

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyProject LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 自动获取LightLog
FetchContent_Declare(
    LightLog
    GIT_REPOSITORY https://github.com/hesphoros/LightLogWriteImplWithPanel.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(LightLog)

# 创建目标并链接
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE LightLog::lightlog)
```

### 方式四：使用find_package

```cmake
# 先安装LightLog到系统
cmake --build build --target install

# 在你的项目中使用
find_package(LightLog REQUIRED)
target_link_libraries(my_app PRIVATE LightLog::lightlog)
```

### CMake选项配置

```cmake
# 可用的配置选项
option(LIGHTLOG_BUILD_EXAMPLES "Build example programs" OFF)
option(LIGHTLOG_BUILD_TESTS "Build test programs" OFF)
option(LIGHTLOG_INSTALL "Generate installation target" OFF)
option(LIGHTLOG_BUILD_SHARED "Build shared library" OFF)

# 在FetchContent中使用
set(LIGHTLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIGHTLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(LightLog)
```

---

## 📖 功能详解

### 🎯 日志级别

LightLog支持10个标准日志级别，从详细到严重：

```cpp
enum class LogLevel {
    Trace = 0,     // 最详细的追踪信息
    Debug,         // 调试信息
    Info,          // 一般信息
    Notice,        // 需要注意的事件
    Warning,       // 警告信息
    Error,         // 错误信息
    Critical,      // 严重错误
    Alert,         // 需要立即处理
    Emergency,     // 系统不可用
    Fatal          // 致命错误
};
```

使用示例：

```cpp
// 方式1：使用便捷方法
logger->WriteLogTrace(L"详细追踪信息");
logger->WriteLogDebug(L"调试信息");
logger->WriteLogInfo(L"一般信息");
logger->WriteLogNotice(L"需要注意的事件");
logger->WriteLogWarning(L"警告");
logger->WriteLogError(L"错误");
logger->WriteLogCritical(L"严重错误");
logger->WriteLogAlert(L"需要立即处理");
logger->WriteLogEmergency(L"系统不可用");
logger->WriteLogFatal(L"致命错误");

// 方式2：使用通用接口
logger->WriteLogContent(LogLevel::Info, L"信息日志");
logger->WriteLogContent(LogLevel::Error, L"错误日志");

// 设置最小日志级别
logger->SetMinLogLevel(LogLevel::Info);  // 只记录Info及以上级别
```

### 🔄 日志轮转系统

支持多种轮转策略，自动管理日志文件：

```cpp
LogRotationConfig config;

// 1. 基于文件大小的轮转
config.strategy = LogRotationStrategy::Size;
config.maxFileSizeMB = 50;              // 50MB触发轮转
config.maxArchiveFiles = 10;            // 保留10个归档文件

// 2. 基于时间的轮转  
config.strategy = LogRotationStrategy::Time;
config.timeInterval = TimeRotationInterval::Daily;   // 每天轮转
// 可选值: Hourly, Daily, Weekly, Monthly

// 3. 复合轮转策略（同时基于大小和时间）
config.strategy = LogRotationStrategy::SizeAndTime;
config.maxFileSizeMB = 100;
config.timeInterval = TimeRotationInterval::Daily;

// 4. 轮转配置选项
config.enableCompression = true;        // 启用压缩
config.enableAsync = true;              // 异步轮转
config.asyncWorkerCount = 2;            // 异步工作线程数
config.archiveDirectory = L"logs/archive"; // 归档目录
config.deleteSourceAfterArchive = true; // 归档后删除源文件

// 应用配置
logger->SetLogRotationConfig(config);

// 手动触发轮转
logger->ForceLogRotation();

// 异步轮转
auto future = logger->ForceLogRotationAsync();
future.wait();  // 等待完成

// 查询当前日志文件大小
size_t currentSize = logger->GetCurrentLogFileSize();
```

### 📦 压缩功能

内置多种压缩算法，显著节省存储空间：

```cpp
// 1. 创建压缩器配置
LogCompressorConfig compressorConfig;
compressorConfig.algorithm = CompressionAlgorithm::ZIP;  // ZIP, GZIP, LZ4, ZSTD
compressorConfig.compressionLevel = 6;                   // 1-9，平衡速度和压缩率
compressorConfig.workerThreadCount = 4;                  // 并行压缩线程数
compressorConfig.maxQueueSize = 1000;                    // 最大队列大小
compressorConfig.deleteSourceAfterSuccess = true;        // 压缩成功后删除源文件
compressorConfig.enableStatistics = true;                // 启用统计功能

// 2. 创建并启动压缩器
auto compressor = std::make_shared<LogCompressor>(compressorConfig);
compressor->Start();

// 3. 将压缩器传递给日志记录器
auto logger = std::make_shared<LightLogWrite_Impl>(
    10000,                              // 队列大小
    LogQueueOverflowStrategy::Block,    // 溢出策略
    1000,                               // 批处理大小
    compressor                          // 压缩器
);

// 4. 获取压缩统计信息
auto stats = logger->GetCompressionStatistics();
if (stats.totalTasks > 0) {
    std::wcout << L"总任务数: " << stats.totalTasks << std::endl;
    std::wcout << L"成功任务数: " << stats.successfulTasks << std::endl;
    std::wcout << L"失败任务数: " << stats.failedTasks << std::endl;
    std::wcout << L"原始总大小: " << stats.totalOriginalSize << " bytes" << std::endl;
    std::wcout << L"压缩后大小: " << stats.totalCompressedSize << " bytes" << std::endl;
    std::wcout << L"平均压缩率: " << (stats.GetAverageCompressionRatio() * 100) << "%" << std::endl;
}
```

### 🔍 过滤器系统

强大的过滤器系统，支持多种过滤方式：

```cpp
#include "log/LogFilters.h"
#include "log/CompositeFilter.h"

// 1. 级别过滤器
auto levelFilter = std::make_shared<LevelFilter>(LogLevel::Warning);
logger->SetLogFilter(levelFilter);

// 2. 关键词过滤器
auto keywordFilter = std::make_shared<KeywordFilter>();
keywordFilter->AddIncludeKeyword(L"error");      // 包含"error"的日志
keywordFilter->AddIncludeKeyword(L"warning");
keywordFilter->AddExcludeKeyword(L"debug");      // 排除"debug"的日志
logger->SetLogFilter(keywordFilter);

// 3. 正则表达式过滤器
auto regexFilter = std::make_shared<RegexFilter>(L"\\b(error|warning|critical)\\b");
logger->SetLogFilter(regexFilter);

// 4. 频率限制过滤器（防止日志洪泛）
auto rateLimitFilter = std::make_shared<RateLimitFilter>(100, 10);  // 100消息/秒，突发10
logger->SetLogFilter(rateLimitFilter);

// 5. 线程过滤器
auto threadFilter = std::make_shared<ThreadFilter>();
threadFilter->AddAllowedThreadId(std::this_thread::get_id());
logger->SetLogFilter(threadFilter);

// 6. 组合过滤器（多个过滤器组合）
auto compositeFilter = std::make_shared<CompositeFilter>(
    L"MainFilter", CompositionStrategy::AllMustPass  // 所有过滤器都通过
);
compositeFilter->AddFilter(levelFilter);
compositeFilter->AddFilter(keywordFilter);
logger->SetLogFilter(compositeFilter);

// 其他组合策略：
// - AnyCanPass: 任一过滤器通过即可
// - MajorityMustPass: 多数通过
// - Custom: 自定义策略

// 清除过滤器
logger->ClearLogFilter();

// 检查是否有过滤器
bool hasFilter = logger->HasLogFilter();
```

### 🎯 多输出系统

支持同时输出到多个目标：

```cpp
#include "log/ConsoleLogOutput.h"
#include "log/FileLogOutput.h"
#include "log/LogOutputManager.h"

// 1. 启用多输出系统
logger->SetMultiOutputEnabled(true);

// 2. 创建控制台输出
auto consoleOutput = std::make_shared<ConsoleLogOutput>(
    L"Console",      // 输出名称
    true,            // 输出到stderr（错误流）
    true,            // 启用颜色
    false            // 不使用单独控制台
);

// 3. 创建文件输出
auto fileOutput = std::make_shared<FileLogOutput>(L"MainFile");
fileOutput->Initialize(L"logs/app.log");

// 4. 添加输出
logger->AddLogOutput(consoleOutput);
logger->AddLogOutput(fileOutput);

// 5. 移除输出
logger->RemoveLogOutput(L"Console");

// 6. 从JSON配置文件加载
logger->LoadMultiOutputConfigFromJson(L"config/log_config.json");

// 7. 保存配置到JSON文件
logger->SaveMultiOutputConfigToJson(L"config/log_config.json");

// 8. 获取输出管理器
auto outputManager = logger->GetOutputManager();
```

### 📞 回调系统

支持日志事件回调通知：

```cpp
// 1. 订阅日志事件
CallbackHandle handle = logger->SubscribeToLogEvents(
    [](const LogCallbackInfo& info) {
        // 回调函数在日志线程上执行，应避免阻塞操作
        std::wcout << L"[Callback] " << info.levelString 
                   << L": " << info.message << std::endl;
    },
    LogLevel::Warning  // 只接收Warning及以上级别
);

// 2. 取消订阅
bool removed = logger->UnsubscribeFromLogEvents(handle);

// 3. 清除所有回调
logger->ClearAllLogCallbacks();

// 4. 获取回调数量
size_t count = logger->GetCallbackCount();
```

---

## 🎮 示例程序

项目包含丰富的示例程序，展示各种功能的使用：

### 主要示例文件

| 示例文件 | 功能说明 |
|---------|---------|
| `examples/simple_demo.cpp` | 基本使用演示 |
| `examples/demo_main.cpp` | 综合功能演示和测试框架 |
| `examples/rotation_demo.cpp` | 日志轮转功能演示 |
| `examples/rotation_compression_examples.cpp` | 轮转和压缩机制演示 |
| `examples/rotation_configuration_examples.cpp` | 轮转配置详解 |
| `examples/rotation_strategy_examples.cpp` | 各种轮转策略演示 |
| `examples/filter_system_demo.cpp` | 过滤器系统演示 |
| `examples/filter_serialization_demo.cpp` | 过滤器序列化演示 |
| `examples/multioutput_json_config_demo.cpp` | 多输出JSON配置演示 |

### 运行示例

```bash
# 构建项目（启用示例）
cd LightLogWriteImplWithPanel
mkdir build && cd build
cmake .. -DLIGHTLOG_BUILD_EXAMPLES=ON
cmake --build . --config Release

# 运行示例程序
# Linux/macOS:
./bin/simple_demo
./bin/demo_main
./bin/rotation_demo

# Windows:
.\bin\Release\simple_demo.exe
.\bin\Release\demo_main.exe
.\bin\Release\rotation_demo.exe
```

---

## 📁 项目结构

```text
LightLogWriteImplWithPanel/
├── CMakeLists.txt                          # 主CMake配置文件
├── LICENSE                                 # GPL v3许可证
├── README.md                               # 项目说明文档
├── CHANGELOG.md                            # 版本变更记录
│
├── include/log/                            # 公共头文件目录
│   ├── LightLogWriteImpl.h                 # 主日志类接口
│   ├── LogCommon.h                         # 公共定义（日志级别、回调等）
│   ├── ILogRotationManager.h               # 轮转管理器接口
│   ├── ILogCompressor.h                    # 压缩器接口
│   ├── LogCompressor.h                     # 压缩器实现
│   ├── ILogFilter.h                        # 过滤器基础接口
│   ├── LogFilters.h                        # 各种过滤器实现
│   ├── CompositeFilter.h                   # 组合过滤器
│   ├── FilterManager.h                     # 过滤器管理器
│   ├── LogFilterFactory.h                  # 过滤器工厂
│   ├── ILogOutput.h                        # 输出接口
│   ├── ConsoleLogOutput.h                  # 控制台输出
│   ├── FileLogOutput.h                     # 文件输出
│   ├── LogOutputManager.h                  # 多输出管理器
│   ├── MultiOutputLogConfig.h              # 多输出配置
│   ├── ILogFormatter.h                     # 格式化器接口
│   ├── BasicLogFormatter.h                 # 基础格式化器
│   ├── RotationManagerFactory.h            # 轮转管理器工厂
│   ├── RotationStrategies.h                # 轮转策略
│   ├── RotationErrorHandler.h              # 轮转错误处理
│   ├── RotationPreChecker.h                # 轮转前置检查
│   ├── RotationStateMachine.h              # 轮转状态机
│   ├── TransactionalRotation.h             # 事务性轮转
│   ├── AsyncRotationManager.h              # 异步轮转管理器
│   ├── TimeCalculator.h                    # 时间计算工具
│   ├── UniConvAdapter.h                    # 字符编码适配器
│   ├── DebugUtils.h                        # 调试工具
│   └── singleton.h                         # 单例模式模板
│
├── src/log/                                # 实现源码目录
│   ├── LightLogWriteImpl.cpp               # 主日志类实现
│   ├── LogCompressor.cpp                   # 压缩器实现
│   ├── LogFilters.cpp                      # 过滤器实现
│   ├── CompositeFilter.cpp                 # 组合过滤器实现
│   ├── FilterManager.cpp                   # 过滤器管理器实现
│   ├── LogFilterFactory.cpp                # 过滤器工厂实现
│   ├── ConsoleLogOutput.cpp                # 控制台输出实现
│   ├── FileLogOutput.cpp                   # 文件输出实现
│   ├── BaseLogOutput.cpp                   # 输出基类实现
│   ├── LogOutputManager.cpp                # 多输出管理器实现
│   ├── MultiOutputLogConfig.cpp            # 多输出配置实现
│   ├── BasicLogFormatter.cpp               # 基础格式化器实现
│   ├── RotationManagerFactory.cpp          # 轮转管理器工厂实现
│   ├── RotationStrategies.cpp              # 轮转策略实现
│   ├── RotationErrorHandler.cpp            # 轮转错误处理实现
│   ├── RotationPreChecker.cpp              # 轮转前置检查实现
│   ├── RotationStateMachine.cpp            # 轮转状态机实现
│   ├── TransactionalRotation.cpp           # 事务性轮转实现
│   ├── AsyncRotationManager.cpp            # 异步轮转管理器实现
│   └── TimeCalculator.cpp                  # 时间计算工具实现
│
├── examples/                               # 示例程序目录
│   ├── simple_demo.cpp                     # 简单演示
│   ├── demo_main.cpp                       # 综合功能演示
│   ├── rotation_demo.cpp                   # 轮转演示
│   ├── rotation_compression_examples.cpp   # 轮转压缩演示
│   ├── rotation_configuration_examples.cpp # 轮转配置演示
│   ├── rotation_strategy_examples.cpp      # 轮转策略演示
│   ├── filter_system_demo.cpp              # 过滤器系统演示
│   ├── filter_serialization_demo.cpp       # 过滤器序列化演示
│   └── multioutput_json_config_demo.cpp    # 多输出JSON配置演示
│
├── test/                                   # 测试程序目录
│   ├── test_rotation.cpp                   # 轮转功能测试
│   └── test_compressor.cpp                 # 压缩器测试
│
├── docs/                                   # 详细文档目录
│   ├── rotation_strategy_guide.md          # 轮转策略指南
│   ├── rotation_strategy_api.md            # 轮转策略API文档
│   ├── rotation_system.md                  # 轮转系统说明
│   ├── enhanced_filter_system.md           # 增强过滤器系统
│   ├── filter_serialization_guide.md       # 过滤器序列化指南
│   ├── multioutput_json_config_guide.md    # 多输出JSON配置指南
│   ├── debug_system_guide.md               # 调试系统指南
│   └── cmake_build_guide.md                # CMake构建指南
│
├── config/                                 # 配置文件示例
│   ├── example_filter_config.json          # 过滤器配置示例
│   └── example_multioutput_config.json     # 多输出配置示例
│
├── example_usage/                          # 集成示例目录
│   ├── README.md                           # 集成使用说明
│   ├── CMakeLists.txt                      # 示例CMake配置
│   └── main.cpp                            # 示例主程序
│
├── cmake/                                  # CMake模块
│   └── ...                                 # CMake辅助脚本
│
└── lib/                                    # 第三方库（包含在项目中）
    ├── json/                               # nlohmann/json
    ├── thread_pool/                        # BS::thread_pool
    └── miniz/                              # miniz压缩库
```

### 关键文件说明

- **主要接口**: `include/log/LightLogWriteImpl.h` - 这是你需要包含的主要头文件
- **日志级别定义**: `include/log/LogCommon.h` - 包含日志级别、回调函数等公共定义
- **演示程序**: `examples/demo_main.cpp` - 包含完整的功能演示和测试框架
- **简单示例**: `examples/simple_demo.cpp` - 最简单的使用示例，适合入门
- **构建配置**: `CMakeLists.txt` - 现代CMake配置，支持FetchContent和多种集成方式
- **文档目录**: `docs/` - 包含详细的API文档和使用指南

---

## 📊 性能特性

### 队列配置

LightLog使用异步队列处理日志，支持两种溢出策略：

```cpp
// 阻塞策略：队列满时阻塞直到有空间
auto logger = std::make_shared<LightLogWrite_Impl>(
    10000,                              // 队列大小
    LogQueueOverflowStrategy::Block,    // 阻塞策略
    1000                                // 批处理大小
);

// 丢弃策略：队列满时丢弃最旧的消息
auto logger = std::make_shared<LightLogWrite_Impl>(
    10000,                              // 队列大小
    LogQueueOverflowStrategy::DropOldest, // 丢弃策略
    1000                                // 批处理大小
);

// 查询丢弃计数
size_t discarded = logger->GetDiscardCount();
logger->ResetDiscardCount();
```

### 实际性能数据

基于Windows 11 + Visual Studio 2022 Release模式测试：

- **写入速度**: 100,000+ 消息/秒 (典型配置)
- **内存占用**: < 10MB (10,000条消息队列)
- **压缩比率**: 90-95% (实测66KB→3KB)
- **轮转耗时**: < 100ms (100MB文件)
- **批处理性能**: 1000条消息 < 100ms

### 压缩效果实测

```text
测试场景: 1000条结构化日志消息
原始大小: 66.2 KB
压缩后大小: 3.0 KB  
压缩率: 95.45%
空间节省: 63.2 KB
压缩时间: < 50ms
```

### 性能优化建议

```cpp
// 1. 高吞吐量配置
auto logger = std::make_shared<LightLogWrite_Impl>(
    100000,                             // 大队列减少阻塞
    LogQueueOverflowStrategy::DropOldest, // 丢弃策略避免阻塞
    5000                                // 大批处理提升吞吐
);

// 2. 轮转优化配置
LogRotationConfig config;
config.enableAsync = true;              // 启用异步轮转
config.asyncWorkerCount = 4;            // 多工作线程

// 3. 压缩优化配置
LogCompressorConfig compConfig;
compConfig.compressionLevel = 1;        // 快速压缩（级别1-3）
compConfig.workerThreadCount = std::thread::hardware_concurrency();

// 4. 禁用不需要的功能
logger->SetMinLogLevel(LogLevel::Info); // 过滤低级别日志
logger->ClearLogFilter();               // 移除复杂过滤器
```

---

## 🔧 配置详解

### JSON配置文件

多输出系统支持JSON配置文件：

```json
{
  "configVersion": "1.0",
  "enabled": true,
  "globalMinLevel": 2,
  "outputs": [
    {
      "name": "MainFile",
      "type": "File",
      "enabled": true,
      "minLevel": 1,
      "filePath": "logs/app.log",
      "rotation": {
        "enabled": true,
        "maxSizeMB": 100,
        "maxFiles": 30,
        "compression": true
      }
    },
    {
      "name": "Console",
      "type": "Console",
      "enabled": true,
      "minLevel": 3,
      "useColors": true,
      "separateConsole": false
    }
  ]
}
```

### 代码配置示例

```cpp
// 1. 从JSON加载配置
bool success = logger->LoadMultiOutputConfigFromJson(L"config/log_config.json");

// 2. 保存配置到JSON
logger->SaveMultiOutputConfigToJson(L"config/log_config_backup.json");

// 3. 代码直接配置输出
auto fileOutput = std::make_shared<FileLogOutput>(L"MyFile");
fileOutput->Initialize(L"logs/my_app.log");
fileOutput->SetMinLevel(LogLevel::Info);
logger->AddLogOutput(fileOutput);
```

---

## 🧪 测试

### 构建测试程序

```bash
# 配置CMake（启用测试）
cd LightLogWriteImplWithPanel
mkdir build && cd build
cmake .. -DLIGHTLOG_BUILD_TESTS=ON

# 构建
cmake --build . --config Release
```

### 运行测试

```bash
# Linux/macOS
./bin/test_rotation
./bin/test_compressor

# Windows  
.\bin\Release\test_rotation.exe
.\bin\Release\test_compressor.exe
```

### 集成测试

主演示程序包含完整的集成测试框架：

```bash
# 运行完整测试套件
# Linux/macOS:
./bin/demo_main

# Windows:
.\bin\Release\demo_main.exe
```

测试模块覆盖范围：

- ✅ **基础日志功能**: 各种日志级别、字符串类型
- ✅ **回调系统**: 事件订阅、取消订阅、多回调
- ✅ **轮转系统**: 大小轮转、时间轮转、复合策略
- ✅ **压缩功能**: 压缩算法、压缩级别、统计信息
- ✅ **过滤器系统**: 级别过滤、关键词过滤、组合过滤
- ✅ **性能测试**: 吞吐量、延迟、内存使用

---

## 📚 API文档

### 核心类和接口

#### LightLogWrite_Impl - 主日志类

主要的日志记录器类，提供所有日志功能。

**构造函数：**
```cpp
LightLogWrite_Impl(
    size_t maxQueueSize = 500000,                     // 最大队列大小
    LogQueueOverflowStrategy strategy = Block,        // 队列溢出策略
    size_t reportInterval = 100,                      // 丢弃报告间隔
    std::shared_ptr<ILogCompressor> compressor = nullptr  // 压缩器（可选）
);
```

**核心方法：**
```cpp
// 文件配置
void SetLastingsLogs(const std::wstring& dir, const std::wstring& baseName);
void SetLogsFileName(const std::wstring& filename);

// 日志记录
void WriteLogContent(LogLevel level, const std::wstring& message);
void WriteLogTrace(const std::wstring& message);
void WriteLogDebug(const std::wstring& message);
void WriteLogInfo(const std::wstring& message);
void WriteLogWarning(const std::wstring& message);
void WriteLogError(const std::wstring& message);
// ... 其他级别类似

// 日志级别控制
void SetMinLogLevel(LogLevel level);
LogLevel GetMinLogLevel() const;

// 轮转配置
void SetLogRotationConfig(const LogRotationConfig& config);
LogRotationConfig GetLogRotationConfig() const;
void ForceLogRotation();
std::future<bool> ForceLogRotationAsync();
size_t GetCurrentLogFileSize() const;

// 回调系统
CallbackHandle SubscribeToLogEvents(const LogCallback& callback, LogLevel minLevel);
bool UnsubscribeFromLogEvents(CallbackHandle handle);
void ClearAllLogCallbacks();
size_t GetCallbackCount() const;

// 压缩器管理
void SetCompressor(std::shared_ptr<ILogCompressor> compressor);
std::shared_ptr<ILogCompressor> GetCompressor() const;
CompressionStatistics GetCompressionStatistics() const;

// 多输出系统
void SetMultiOutputEnabled(bool enabled);
bool AddLogOutput(std::shared_ptr<ILogOutput> output);
bool RemoveLogOutput(const std::wstring& outputName);
bool LoadMultiOutputConfigFromJson(const std::wstring& configPath);
bool SaveMultiOutputConfigToJson(const std::wstring& configPath);

// 过滤器系统
void SetLogFilter(std::shared_ptr<ILogFilter> filter);
std::shared_ptr<ILogFilter> GetLogFilter() const;
void ClearLogFilter();
bool HasLogFilter() const;

// 队列管理
size_t GetDiscardCount() const;
void ResetDiscardCount();
```

### 详细文档

更多详细信息请参考以下文档：

- **[增强过滤器系统](docs/enhanced_filter_system.md)** - 过滤器的完整使用指南
- **[轮转策略指南](docs/rotation_strategy_guide.md)** - 日志轮转策略详解
- **[轮转系统API](docs/rotation_strategy_api.md)** - 轮转系统API参考
- **[轮转系统说明](docs/rotation_system.md)** - 轮转系统架构说明
- **[多输出配置](docs/multioutput_json_config_guide.md)** - JSON配置指南
- **[过滤器序列化](docs/filter_serialization_guide.md)** - 过滤器序列化指南
- **[调试系统](docs/debug_system_guide.md)** - 调试功能指南
- **[CMake构建](docs/cmake_build_guide.md)** - CMake构建配置详解

---

## 🤝 贡献指南

我们欢迎各种形式的贡献：

### 如何贡献

1. **Fork** 项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 **Pull Request**

### 开发环境设置

```bash
# 克隆仓库
git clone https://github.com/your-username/LightLogWriteImplWithPanel.git

# 创建开发分支
git checkout -b develop

# 安装开发依赖 (如需要)
# 配置IDE或编辑器
```

### 代码规范

- 遵循现有代码风格
- 添加适当的注释和文档
- 确保所有测试通过
- 新功能需要添加相应测试

---

## 🐛 问题报告

遇到问题？请通过以下方式报告：

1. 查看 [现有Issues](https://github.com/hesphoros/LightLogWriteImplWithPanel/issues)
2. 创建新Issue，请提供：
   - 问题描述
   - 复现步骤
   - 预期行为
   - 实际行为
   - 环境信息（操作系统、编译器版本等）

---

## 📈 路线图

### 版本 1.1.0 (计划中)

- [ ] macOS平台支持
- [ ] 更多过滤器类型
- [ ] 网络日志输出
- [ ] 性能监控面板

### 版本 1.2.0 (计划中)

- [ ] Python绑定
- [ ] C#绑定
- [ ] 配置热重载
- [ ] 分布式日志收集

### 长期规划

- [ ] 云原生支持
- [ ] Kubernetes集成
- [ ] 监控和可观测性增强
- [ ] 图形化管理界面

---

## 📄 许可证

本项目采用 [GPL v3 许可证](LICENSE) - 详情请查看LICENSE文件。

---

## 👨‍💻 作者

**hesphoros** - *项目创始人和主要开发者*

- 📧 Email: <hesphoros@gmail.com>
- 🐙 GitHub: [@hesphoros](https://github.com/hesphoros)

---

## 🙏 致谢

特别感谢以下开源项目和技术：

- **[nlohmann/json](https://github.com/nlohmann/json)** - 优秀的JSON库，用于配置文件解析
- **[BS::thread_pool](https://github.com/bshoshany/thread-pool)** - 高性能线程池，用于异步任务处理
- **[UniConv](https://github.com/hesphoros/UniConv)** - 字符编码转换库
- **[miniz](https://github.com/richgel999/miniz)** - 轻量级ZIP压缩库，提供压缩功能

感谢所有为本项目贡献代码、报告问题和提供建议的开发者！

---

## 📞 支持

如需支持，请：

1. 查看 [文档](docs/) 和 [示例](examples/)
2. 搜索 [已有Issues](https://github.com/hesphoros/LightLogWriteImplWithPanel/issues)
3. 创建新Issue或发送邮件至 <hesphoros@gmail.com>

### 常见问题

**Q: 如何在多线程环境中使用？**  
A: LightLog完全线程安全，可以从多个线程同时调用日志方法，无需额外同步。

**Q: 日志文件会自动轮转吗？**  
A: 是的，配置好轮转策略后，日志会自动轮转。你也可以手动调用`ForceLogRotation()`强制轮转。

**Q: 压缩功能会影响性能吗？**  
A: 压缩是异步进行的，不会阻塞日志写入。你可以调整`workerThreadCount`和`compressionLevel`来平衡性能和压缩率。

**Q: 如何只记录错误和警告级别的日志？**  
A: 使用`SetMinLogLevel(LogLevel::Warning)`或设置级别过滤器。

**Q: 支持Unicode字符吗？**  
A: 完全支持，建议使用`std::wstring`接口来处理Unicode文本。

---

---

**⭐ 如果这个项目对你有帮助，请给个星星！**

[⬆ 回到顶部](#lightlog---modern-c17-logging-library)

