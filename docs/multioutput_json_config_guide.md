# Multi-Output JSON Configuration Guide

## 概述

`LightLogWriteImpl` 现在支持完整的JSON配置序列化和反序列化功能，允许您轻松保存和加载复杂的多输出日志配置。

## ✅ 功能特性

### 已实现功能
- ✅ **完整JSON序列化**：将所有配置转换为结构化JSON
- ✅ **完整JSON反序列化**：从JSON文件恢复所有配置
- ✅ **多输出支持**：支持控制台、文件、网络等多种输出类型
- ✅ **格式化器配置**：完整的日志格式化设置
- ✅ **管理器配置**：并发模式、队列大小、线程数等设置
- ✅ **UTF-8编码支持**：正确处理中文和其他Unicode字符
- ✅ **错误处理**：完善的异常处理和默认值恢复
- ✅ **版本兼容**：配置版本标识，支持向后兼容

### 待完善功能
- 🔄 **过滤器配置**：过滤器系统序列化（预留接口）
- 🔄 **网络输出配置**：网络日志输出的特殊配置
- 🔄 **自定义输出类型**：插件式输出类型支持

## 🚀 使用方法

### 基础使用

```cpp
#include "include/log/LightLogWriteImpl.h"

// 创建日志实例
auto logger = std::make_shared<LightLogWrite_Impl>();
logger->SetMultiOutputEnabled(true);

// 添加一些输出
auto consoleOutput = std::make_shared<ConsoleLogOutput>(L"Console");
logger->AddLogOutput(consoleOutput);

// 保存配置到JSON文件
if (logger->SaveMultiOutputConfigToJson(L"config/my_log_config.json")) {
    std::wcout << L"✓ 配置保存成功" << std::endl;
}

// 从JSON文件加载配置
if (logger->LoadMultiOutputConfigFromJson(L"config/my_log_config.json")) {
    std::wcout << L"✓ 配置加载成功" << std::endl;
}
```

### 高级配置示例

```cpp
// 直接操作配置对象
MultiOutputLogConfig config;

// 基础设置
config.enabled = true;
config.configVersion = L"2.0";
config.globalMinLevel = LogLevel::Debug;

// 管理器配置
config.managerConfig.writeMode = OutputWriteMode::Parallel;
config.managerConfig.asyncQueueSize = 2000;
config.managerConfig.workerThreadCount = 4;
config.managerConfig.failFastOnError = false;
config.managerConfig.writeTimeout = 10.0;

// 控制台输出配置
OutputConfig consoleConfig;
consoleConfig.name = L"MainConsole";
consoleConfig.type = L"Console";
consoleConfig.enabled = true;
consoleConfig.minLevel = LogLevel::Info;

// 格式化器配置
consoleConfig.useFormatter = true;
consoleConfig.formatterConfig.pattern = L"[{timestamp}] [{level}] {message}";
consoleConfig.formatterConfig.enableColors = true;
consoleConfig.formatterConfig.levelColors[LogLevel::Error] = LogColor::Red;

config.outputs.push_back(consoleConfig);

// 保存配置
MultiOutputConfigSerializer::SaveToFile(config, L"advanced_config.json");
```

## 📄 JSON配置文件结构

### 完整配置示例

```json
{
    "configVersion": "2.0",
    "enabled": true,
    "globalMinLevel": "Info",
    "managerConfig": {
        "writeMode": "Parallel",
        "asyncQueueSize": 2000,
        "workerThreadCount": 4,
        "failFastOnError": false,
        "writeTimeout": 10.0
    },
    "outputs": [
        {
            "name": "ConsoleOutput",
            "type": "Console",
            "enabled": true,
            "minLevel": "Debug",
            "config": "useStderr=false;enableColors=true",
            "useFormatter": true,
            "formatterConfig": {
                "pattern": "[{timestamp}] [{level}] {message}",
                "timestampFormat": "%Y-%m-%d %H:%M:%S",
                "enableColors": true,
                "enableThreadId": false,
                "enableProcessId": false,
                "enableSourceInfo": false,
                "levelColors": {
                    "Info": 2,
                    "Warning": 3,
                    "Error": 1,
                    "Critical": 10
                }
            },
            "useFilter": false
        }
    ]
}
```

### 配置字段说明

#### 根级别配置
- `configVersion`: 配置版本号
- `enabled`: 全局启用/禁用标志
- `globalMinLevel`: 全局最小日志级别
- `managerConfig`: 输出管理器配置
- `outputs`: 输出配置数组

#### 管理器配置 (managerConfig)
- `writeMode`: 写入模式 (`Sequential`/`Parallel`/`Async`)
- `asyncQueueSize`: 异步队列大小
- `workerThreadCount`: 工作线程数
- `failFastOnError`: 遇到错误时是否快速失败
- `writeTimeout`: 写入超时时间（秒）

#### 输出配置 (outputs[])
- `name`: 输出名称（唯一标识）
- `type`: 输出类型 (`Console`/`File`/`Network`等)
- `enabled`: 是否启用此输出
- `minLevel`: 此输出的最小日志级别
- `config`: 输出特定配置字符串
- `useFormatter`: 是否使用格式化器
- `formatterConfig`: 格式化器详细配置
- `useFilter`: 是否使用过滤器（预留）

#### 格式化器配置 (formatterConfig)
- `pattern`: 日志格式模式字符串
- `timestampFormat`: 时间戳格式
- `enableColors`: 是否启用颜色
- `enableThreadId`: 是否包含线程ID
- `enableProcessId`: 是否包含进程ID
- `enableSourceInfo`: 是否包含源码信息
- `levelColors`: 日志级别颜色映射

## 🎨 日志级别和颜色

### 日志级别
- `Trace`: 跟踪信息
- `Debug`: 调试信息
- `Info`: 普通信息
- `Notice`: 注意信息
- `Warning`: 警告信息
- `Error`: 错误信息
- `Critical`: 严重错误
- `Alert`: 告警信息
- `Emergency`: 紧急信息
- `Fatal`: 致命错误

### 颜色代码
- `0`: Default (默认)
- `1`: Red (红色)
- `2`: Green (绿色)
- `3`: Yellow (黄色)
- `4`: Blue (蓝色)
- `5`: Magenta (品红)
- `6`: Cyan (青色)
- `7`: White (白色)
- `10`: BrightRed (亮红色)
- `11`: BrightGreen (亮绿色)
- `12`: BrightYellow (亮黄色)

## 🔧 最佳实践

### 1. 配置文件管理
```cpp
// 使用相对路径，便于部署
const std::wstring CONFIG_PATH = L"config/log_config.json";

// 检查文件是否存在
if (std::filesystem::exists(CONFIG_PATH)) {
    logger->LoadMultiOutputConfigFromJson(CONFIG_PATH);
} else {
    // 创建默认配置
    SetupDefaultConfiguration(logger);
    logger->SaveMultiOutputConfigToJson(CONFIG_PATH);
}
```

### 2. 错误处理
```cpp
try {
    if (!logger->SaveMultiOutputConfigToJson(configPath)) {
        std::wcerr << L"警告：无法保存日志配置" << std::endl;
        // 使用默认配置继续运行
    }
} catch (const std::exception& e) {
    std::wcerr << L"配置保存异常: " << e.what() << std::endl;
}
```

### 3. 配置验证
```cpp
// 加载后验证配置
if (logger->LoadMultiOutputConfigFromJson(configPath)) {
    auto outputManager = logger->GetOutputManager();
    if (outputManager->GetOutputCount() == 0) {
        std::wcout << L"警告：没有可用的日志输出" << std::endl;
        // 添加默认输出
    }
}
```

### 4. 动态配置
```cpp
// 运行时更新配置
void UpdateLogConfiguration(const std::wstring& newConfigPath) {
    // 保存当前状态
    bool wasEnabled = logger->IsMultiOutputEnabled();
    
    // 加载新配置
    if (logger->LoadMultiOutputConfigFromJson(newConfigPath)) {
        logger->SetMultiOutputEnabled(wasEnabled);
        logger->WriteLogInfo(L"日志配置已更新");
    } else {
        logger->WriteLogError(L"无法加载新的日志配置");
    }
}
```

## 🐛 故障排除

### 常见问题

1. **文件权限错误**
   - 确保目录存在且有写权限
   - 使用 `std::filesystem::create_directories()` 创建目录

2. **编码问题**
   - JSON文件使用UTF-8编码
   - 中文字符会被正确处理

3. **配置加载失败**
   - 检查JSON格式是否正确
   - 使用在线JSON验证工具验证

4. **输出不工作**
   - 检查 `enabled` 字段是否为 `true`
   - 验证 `minLevel` 设置是否正确

### 调试提示

```cpp
// 启用详细错误信息
try {
    auto config = MultiOutputConfigSerializer::FromJson(jsonData);
    // 处理配置...
} catch (const nlohmann::json::exception& e) {
    std::wcout << L"JSON解析错误: " << e.what() << std::endl;
} catch (const std::exception& e) {
    std::wcout << L"配置处理错误: " << e.what() << std::endl;
}
```

## 📚 示例代码

完整的使用示例请参考：
- `examples/multioutput_json_config_demo.cpp` - 基础和高级用法演示
- `config/example_multioutput_config.json` - 完整配置文件示例

## 🔮 未来计划

- [ ] **可视化配置编辑器**：Web界面配置编辑
- [ ] **配置模板系统**：预定义配置模板
- [ ] **热重载支持**：文件变化时自动重新加载
- [ ] **配置校验**：更严格的配置验证
- [ ] **性能监控集成**：配置性能指标收集

---

**注意**：此JSON配置系统已经完全实现并可以正常使用。所有核心功能都已经过测试，可以安全地在生产环境中使用。