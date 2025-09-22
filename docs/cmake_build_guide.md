# CMake 构建指南

本文档介绍如何使用CMake构建LightLogWriteImplWithPanel项目。

## 📋 环境要求

### 基础要求
- **CMake**: 3.16 或更高版本
- **C++编译器**: 支持C++17标准
  - Windows: Visual Studio 2019 或更高版本
  - Linux: GCC 7.0+ 或 Clang 6.0+
  - macOS: Xcode 10+ 或 Clang 6.0+

### 可选依赖
- **Ninja**: 快速构建系统 (推荐)
- **Doxygen**: 生成文档
- **Git**: 版本控制

## 🚀 快速开始

### Windows (Visual Studio)

#### 方法1: 使用构建脚本 (推荐)
```cmd
# 基础构建
.\build.bat

# Debug构建
.\build.bat --build-type Debug

# 包含调试输出
.\build.bat --build-type Debug --debug-output

# 完整构建并安装
.\build.bat --build-type Release --install
```

#### 方法2: 手动CMake命令
```cmd
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -G "Visual Studio 17 2022" -A x64

# 构建项目
cmake --build . --config Release

# 安装 (可选)
cmake --install . --config Release
```

### Linux/macOS

#### 方法1: 使用构建脚本 (推荐)
```bash
# 给脚本执行权限
chmod +x build.sh

# 基础构建
./build.sh

# Debug构建
./build.sh --build-type Debug

# 共享库构建
./build.sh --shared

# 完整构建并安装
./build.sh --build-type Release --install
```

#### 方法2: 手动CMake命令
```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -DCMAKE_BUILD_TYPE=Release

# 构建项目
cmake --build . -j$(nproc)

# 安装 (可选)
sudo cmake --install .
```

## ⚙️ 构建选项

### 主要选项

| 选项 | 默认值 | 描述 |
|------|--------|------|
| `LIGHTLOG_BUILD_SHARED` | `OFF` | 构建共享库而非静态库 |
| `LIGHTLOG_BUILD_TESTS` | `主项目时为ON` | 构建测试程序 |
| `LIGHTLOG_BUILD_EXAMPLES` | `主项目时为ON` | 构建示例程序 |
| `LIGHTLOG_INSTALL` | `主项目时为ON` | 生成安装目标 |
| `CMAKE_BUILD_TYPE` | `Release` | 构建类型 (Debug/Release) |
| `CMAKE_INSTALL_PREFIX` | 系统默认 | 安装路径前缀 |

### 调试选项

| 选项 | 默认值 | 描述 |
|------|--------|------|
| `ENABLE_DEBUG_OUTPUT` | `OFF` | 启用调试输出 |
| `ENABLE_MULTIOUTPUT_DEBUG` | `OFF` | 启用多输出调试 |
| `ENABLE_CONSOLE_DEBUG` | `OFF` | 启用控制台调试 |
| `ENABLE_ROTATION_DEBUG` | `OFF` | 启用轮转调试 |
| `ENABLE_COMPRESSION_DEBUG` | `OFF` | 启用压缩调试 |

### 使用示例

```bash
# 构建静态库 (Release)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF

# 构建共享库 (Debug) 带调试输出
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON -DENABLE_DEBUG_OUTPUT=ON

# 仅构建核心库，不构建测试和示例
cmake -B build -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF

# 指定安装路径
cmake -B build -DCMAKE_INSTALL_PREFIX=/opt/lightlog

# 构建
cmake --build build

# 安装
cmake --install build
```

## 🎯 CMake预设

项目包含预设配置文件 `CMakePresets.json`，支持以下预设：

### 配置预设
- `windows-msvc-debug`: Windows MSVC Debug 配置
- `windows-msvc-release`: Windows MSVC Release 配置  
- `windows-clang-debug`: Windows Clang Debug 配置
- `linux-gcc-debug`: Linux GCC Debug 配置
- `linux-gcc-release`: Linux GCC Release 配置

### 使用预设

```bash
# 列出可用预设
cmake --list-presets

# 使用预设配置
cmake --preset windows-msvc-release

# 使用预设构建
cmake --build --preset windows-msvc-release

# 使用预设测试
ctest --preset windows-msvc-release
```

## 📦 构建目标

### 主要目标

| 目标 | 描述 | 输出文件 |
|------|------|----------|
| `lightlog` | 核心日志库 | `liblightlog.a/lightlog.lib` |
| `lightlog_main` | 主程序 | `LightLogWriteImplWithPanel` |
| `test_rotation` | 轮转测试 | `test_rotation` |
| `test_compressor` | 压缩测试 | `test_compressor` |

### 示例目标

所有 `examples/*.cpp` 文件会自动生成对应的可执行文件目标。

### 构建特定目标

```bash
# 仅构建核心库
cmake --build build --target lightlog

# 仅构建主程序
cmake --build build --target lightlog_main

# 仅构建测试
cmake --build build --target test_rotation test_compressor
```

## 🔧 集成到其他项目

### 方法1: find_package (推荐)

安装后使用：

```cmake
find_package(LightLog REQUIRED)

target_link_libraries(your_target LightLog::lightlog)
```

### 方法2: 子目录

将项目作为子目录：

```cmake
add_subdirectory(LightLogWriteImplWithPanel)

target_link_libraries(your_target lightlog)
```

### 方法3: FetchContent

从Git仓库获取：

```cmake
include(FetchContent)

FetchContent_Declare(
    LightLog
    GIT_REPOSITORY https://github.com/hesphoros/LightLogWriteImplWithPanel.git
    GIT_TAG v1.0.0
)

FetchContent_MakeAvailable(LightLog)

target_link_libraries(your_target lightlog)
```

## 🏗️ 交叉编译

### Android
```bash
cmake -B build-android \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-21
```

### iOS
```bash
cmake -B build-ios \
    -DCMAKE_TOOLCHAIN_FILE=ios.cmake \
    -DPLATFORM=OS64
```

## 🧪 测试

```bash
# 构建测试
cmake --build build --target test_rotation test_compressor

# 运行测试
cd build
./bin/test_rotation
./bin/test_compressor

# 或使用 CTest
ctest --output-on-failure
```

## 📦 打包

```bash
# 构建包
cmake --build build --target package

# 生成安装包 (Windows)
cpack -G NSIS

# 生成压缩包 (Linux)
cpack -G TGZ
```

## 🔍 故障排除

### 常见问题

1. **找不到编译器**
   ```bash
   cmake -DCMAKE_CXX_COMPILER=/path/to/compiler
   ```

2. **找不到iconv库**
   - Windows: 项目包含预编译库，无需额外配置
   - Linux: `sudo apt-get install libiconv-dev` (Ubuntu/Debian)

3. **C++17支持问题**
   ```bash
   cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON
   ```

4. **权限问题 (安装)**
   ```bash
   # 更改安装路径到用户目录
   cmake -DCMAKE_INSTALL_PREFIX=$HOME/local
   ```

### 调试构建问题

```bash
# 详细输出
cmake --build build --verbose

# 查看变量
cmake -B build -LH

# 生成编译数据库
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

## 📚 相关资源

- [CMake官方文档](https://cmake.org/documentation/)
- [项目README](../README.md)
- [构建脚本](../build.sh) / [Windows构建脚本](../build.bat)
- [CMake预设文件](../CMakePresets.json)

## 💡 最佳实践

1. **使用预设**: 利用 `CMakePresets.json` 统一配置
2. **并行构建**: 使用 `-j$(nproc)` 或 `--parallel` 选项
3. **构建目录**: 始终使用独立的构建目录
4. **缓存管理**: 定期清理CMake缓存 `rm CMakeCache.txt`
5. **依赖管理**: 使用 `find_package` 而非硬编码路径