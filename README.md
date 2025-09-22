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

LightLog 是一个现代化的企业级C++17日志库，提供全面的日志记录、轮转、压缩、过滤和多输出功能。采用现代CMake构建系统，支持多种集成方式，专为高并发、大规模应用场景设计。

### ✨ 核心特性

- 🚀 **高性能**: 异步处理、多线程优化、零拷贝设计
- 🔄 **智能轮转**: 支持大小、时间、复合等多种轮转策略
- 📦 **自动压缩**: 内置ZIP压缩，节省存储空间
- 🔍 **强大过滤**: 级别、关键词、正则表达式、频率限制等多种过滤方式
- 🎯 **多输出支持**: 控制台、文件、网络等多种输出目标
- 🔒 **线程安全**: 完整的多线程支持和线程安全保证
- 🌐 **跨平台**: 支持Windows和Linux平台
- 🏗️ **现代CMake**: 支持FetchContent、add_subdirectory、find_package三种集成方式
- ⚙️ **易配置**: 灵活的JSON配置系统和丰富的API
- 📦 **零依赖**: 自动管理所有外部依赖，开箱即用

---

## 🚀 快速开始

### 基础使用

```cpp
#include "log/LightLogWriteImpl.h"

int main() {
    // 创建日志实例
    auto logger = std::make_shared<LightLogWrite_Impl>();
    
    // 设置日志文件
    logger->SetLastingsLogs(L"logs", L"app");
    
    // 写入不同级别的日志
    logger->WriteLogInfo(L"应用程序启动");
    logger->WriteLogWarning(L"这是一个警告消息");
    logger->WriteLogError(L"发生了一个错误");
    
    return 0;
}
```

### 高级配置示例

```cpp
#include "log/LightLogWriteImpl.h"
#include "log/LogCompressor.h"

int main() {
    // 创建压缩器
    LogCompressorConfig compressorConfig;
    compressorConfig.algorithm = CompressionAlgorithm::ZIP;
    compressorConfig.compressionLevel = 6;
    auto compressor = std::make_shared<LogCompressor>(compressorConfig);
    
    // 创建日志系统
    auto logger = std::make_shared<LightLogWrite_Impl>(
        10000,                              // 队列大小
        LogQueueOverflowStrategy::Block,    // 溢出策略
        1000,                               // 批处理大小
        compressor                          // 压缩器
    );
    
    // 配置轮转
    LogRotationConfig rotationConfig;
    rotationConfig.strategy = LogRotationStrategy::SizeAndTime;
    rotationConfig.maxFileSizeMB = 100;
    rotationConfig.timeInterval = TimeRotationInterval::Daily;
    rotationConfig.enableCompression = true;
    logger->SetLogRotationConfig(rotationConfig);
    
    // 设置过滤器
    auto filter = std::make_unique<LevelFilter>(LogLevel::Info);
    logger->SetLogFilter(std::move(filter));
    
    // 使用日志系统
    logger->WriteLogInfo(L"系统配置完成");
    
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

## � 快速开始

### 1. 基本使用

```cpp
#include "log/LightLogWriteImpl.h"

int main() {
    // 创建日志实例
    LightLogWriteImpl logger;
    
    // 基本日志记录
    logger.WriteLog("Hello, LightLog!", LOG_LEVEL_INFO);
    
    return 0;
}
```

### 2. CMake项目集成示例

完整的项目结构：

```text
my_project/
├── CMakeLists.txt
├── main.cpp
└── external/
    └── LightLog/  # 如果使用子目录方式
```

main.cpp示例：

```cpp
#include "log/LightLogWriteImpl.h"
#include <iostream>
#include <chrono>

int main() {
    LightLogWriteImpl logger;
    
    // 设置日志配置
    logger.SetLogLevel(LOG_LEVEL_DEBUG);
    logger.SetOutputPath("./logs/");
    
    // 记录不同级别的日志
    logger.WriteLog("应用程序启动", LOG_LEVEL_INFO);
    logger.WriteLog("调试信息", LOG_LEVEL_DEBUG);
    logger.WriteLog("警告信息", LOG_LEVEL_WARNING);
    
    // 性能测试
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        logger.WriteLog("性能测试消息 " + std::to_string(i), LOG_LEVEL_INFO);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "1000条日志用时: " << duration.count() << "ms" << std::endl;
    
    return 0;
}
```

项目的CMakeLists.txt：

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 使用FetchContent获取LightLog
include(FetchContent)
FetchContent_Declare(
    LightLog
    GIT_REPOSITORY https://github.com/hesphoros/LightLogWriteImplWithPanel.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

# 配置选项（可选）
set(LIGHTLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIGHTLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(LightLog)

# 创建可执行文件
add_executable(my_app main.cpp)

# 链接LightLog库
target_link_libraries(my_app PRIVATE LightLog::lightlog)

# 可选：设置输出目录
set_target_properties(my_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
```

---

## �📖 功能详解

### 🔄 日志轮转系统

支持多种轮转策略，自动管理日志文件：

```cpp
// 基于文件大小的轮转
LogRotationConfig config;
config.strategy = LogRotationStrategy::Size;
config.maxFileSizeMB = 50;              // 50MB触发轮转
config.maxBackupFiles = 10;             // 保留10个备份

// 基于时间的轮转  
config.strategy = LogRotationStrategy::Time;
config.timeInterval = TimeRotationInterval::Daily;  // 每天轮转

// 复合轮转策略
config.strategy = LogRotationStrategy::SizeAndTime;
```

### 📦 压缩功能

内置ZIP压缩，显著节省存储空间：

```cpp
LogCompressorConfig compressorConfig;
compressorConfig.algorithm = CompressionAlgorithm::ZIP;
compressorConfig.compressionLevel = 6;  // 压缩级别 1-9
compressorConfig.workerThreadCount = 4; // 并行压缩线程数
```

### 🔍 过滤器系统

强大的过滤器系统，支持多种过滤方式：

```cpp
// 级别过滤器
auto levelFilter = std::make_unique<LevelFilter>(LogLevel::Warning);

// 关键词过滤器
auto keywordFilter = std::make_unique<KeywordFilter>();
keywordFilter->AddIncludeKeyword(L"error");
keywordFilter->AddExcludeKeyword(L"debug");

// 正则表达式过滤器
auto regexFilter = std::make_unique<RegexFilter>(L"\\b(error|warning)\\b");

// 频率限制过滤器
auto rateLimitFilter = std::make_unique<RateLimitFilter>(100, 10); // 100/秒，突发10

// 组合过滤器
auto compositeFilter = std::make_unique<CompositeFilter>(
    L"MyFilter", CompositionStrategy::AllMustPass);
compositeFilter->AddFilter(std::move(levelFilter));
compositeFilter->AddFilter(std::move(keywordFilter));
```

### 🎯 多输出系统

支持同时输出到多个目标：

```cpp
// 启用多输出
logger->SetMultiOutputEnabled(true);

// 控制台输出
auto consoleOutput = std::make_shared<ConsoleLogOutput>(
    L"Console", true, true, false);  // 名称、错误输出、颜色、分离控制台

// 文件输出
auto fileOutput = std::make_shared<FileLogOutput>(L"MainFile");

// 添加输出目标
logger->AddLogOutput(consoleOutput);
logger->AddLogOutput(fileOutput);
```

---

## 🎮 示例程序

项目包含丰富的示例程序，展示各种功能的使用：

- **基础使用**: `examples/rotation_demo.cpp`
- **轮转配置**: `examples/rotation_configuration_examples.cpp`
- **压缩功能**: `examples/rotation_compression_examples.cpp`
- **过滤器系统**: `examples/filter_system_demo.cpp`
- **多输出配置**: `examples/multioutput_json_config_demo.cpp`

编译并运行示例：

```bash
# Windows (Visual Studio)
# 在IDE中直接编译运行各示例项目

# Linux
cd examples
g++ -std=c++17 -I../include rotation_demo.cpp -o rotation_demo
./rotation_demo
```

---

## � 项目结构

```text
LightLogWriteImplWithPanel/
├── CMakeLists.txt              # 现代CMake构建配置 
├── src/log/                    # 核心实现源码
│   ├── LightLogWriteImpl.cpp   # 主日志类实现
│   ├── LogRotationManager.cpp  # 轮转管理器
│   ├── LogCompressor.cpp       # 压缩功能
│   └── LogFilters.cpp          # 过滤器系统
├── include/log/                # 公共头文件
│   ├── LightLogWriteImpl.h     # 主接口
│   ├── ILogRotationManager.h   # 轮转接口
│   ├── ILogCompressor.h        # 压缩接口
│   └── LogFilters.h            # 过滤器
├── examples/                   # 示例和演示
│   ├── demo_main.cpp           # 综合功能演示
│   ├── rotation_demo.cpp       # 轮转功能演示
│   └── filter_system_demo.cpp  # 过滤器演示
├── test/                       # 单元测试
│   ├── test_rotation.cpp       # 轮转测试
│   └── test_compressor.cpp     # 压缩测试
├── docs/                       # 详细文档
│   ├── rotation_strategy_guide.md
│   ├── enhanced_filter_system.md
│   └── multioutput_json_config_guide.md
├── config/                     # 配置示例
│   ├── example_filter_config.json
│   └── example_multioutput_config.json
└── lib/                        # 第三方库
    └── iconv/                  # 字符编码支持
```

### 文件说明

- **主要接口**: `include/log/LightLogWriteImpl.h` - 这是你需要包含的主要头文件
- **演示程序**: `examples/demo_main.cpp` - 包含完整的功能演示和性能测试
- **构建配置**: `CMakeLists.txt` - 现代CMake配置，支持FetchContent
- **文档目录**: `docs/` - 包含详细的API文档和使用指南

---

## �📊 性能特性

### 实际性能数据

基于Windows 11 + Visual Studio 2022 Release模式测试：

- **写入速度**: 100,000+ 消息/秒 (典型配置)
- **内存占用**: < 5MB (10,000条消息队列)
- **压缩比率**: 95%+ (实测66KB→3KB)
- **轮转耗时**: < 100ms (1GB文件)
- **批量性能**: 1000条消息 < 100ms

### 压缩效果实测

```text
测试场景: 1000条结构化日志消息
原始大小: 66.2 KB
压缩后: 3.0 KB  
压缩率: 95.45%
压缩时间: < 50ms
```

### 性能优化建议

```cpp
// 高性能配置
LogRotationConfig config;
config.asyncRotation = true;        // 启用异步轮转
config.asyncCompression = true;     // 启用异步压缩
config.compressionLevel = 1;        // 使用快速压缩

// 队列配置优化
auto logger = std::make_shared<LightLogWrite_Impl>(
    100000,                             // 大队列减少阻塞
    LogQueueOverflowStrategy::Drop,     // 丢弃策略避免阻塞
    5000                                // 大批处理提升吞吐
);
```

---

## 🔧 配置详解

### JSON配置文件

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

### 代码配置

```cpp
// 从JSON加载配置
MultiOutputLogConfig config;
if (config.LoadFromFile(L"config/log_config.json")) {
    logger->ApplyMultiOutputConfig(config);
}

// 代码直接配置
LogOutputConfig outputConfig;
outputConfig.name = L"MyOutput";
outputConfig.type = LogOutputType::File;
outputConfig.enabled = true;
outputConfig.minLevel = LogLevel::Info;
outputConfig.filePath = L"logs/my_app.log";
```

---

## 🧪 测试

### 运行测试

```bash
# Windows
.\x64\Release\test_rotation.exe
.\x64\Release\test_compressor.exe

# Linux  
./build/test_rotation
./build/test_compressor
```

### 集成测试

主程序包含完整的集成测试框架：

```bash
# 运行完整测试套件
.\x64\Release\LightLogWriteImplWithPanel.exe
```

测试覆盖范围：

- ✅ 基础日志功能
- ✅ 回调系统
- ✅ 轮转系统  
- ✅ 压缩功能
- ✅ 过滤器系统
- ✅ 性能测试

---

## 📚 API文档

详细的API文档请参考：

- **增强过滤器系统**: [docs/enhanced_filter_system.md](docs/enhanced_filter_system.md)
- **轮转策略指南**: [docs/rotation_strategy_guide.md](docs/rotation_strategy_guide.md)
- **多输出配置**: [docs/multioutput_json_config_guide.md](docs/multioutput_json_config_guide.md)
- **调试系统**: [docs/debug_system_guide.md](docs/debug_system_guide.md)
- **轮转系统API**: [docs/rotation_strategy_api.md](docs/rotation_strategy_api.md)

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

特别感谢以下开源项目：

- [nlohmann/json](https://github.com/nlohmann/json) - 优秀的JSON库
- [BS::thread_pool](https://github.com/bshoshany/thread-pool) - 高性能线程池
- [libiconv](https://www.gnu.org/software/libiconv/) - 字符编码转换
- [miniz](https://github.com/richgel999/miniz) - 轻量级ZIP压缩库

---

## 📞 支持

如需支持，请：

1. 查看 [文档](docs/) 和 [示例](examples/)
2. 搜索 [已有Issues](https://github.com/hesphoros/LightLogWriteImplWithPanel/issues)
3. 创建新Issue或发送邮件

---

---

**⭐ 如果这个项目对你有帮助，请给个星星！**

[⬆ 回到顶部](#lightlog---modern-c17-logging-library)

