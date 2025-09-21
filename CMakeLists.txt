# ====================================================================
# LightLogWriteImplWithPanel - 最简CMake配置
# ====================================================================

cmake_minimum_required(VERSION 3.16)

project(LightLogWriteImplWithPanel
    VERSION 1.0.0
    LANGUAGES CXX
)

# 引入FetchContent模块
include(FetchContent)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows 设置
if(WIN32 AND MSVC)
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/utf-8 /wd4996)
endif()

# 获取并安装UniConv库
FetchContent_Declare(
    UniConv
    GIT_REPOSITORY https://github.com/hesphoros/UniConv.git
    GIT_TAG main
)

FetchContent_MakeAvailable(UniConv)

# 确保UniConv安装到项目中
set_target_properties(UniConv PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

# 包含目录
include_directories(
    include
    include/log
    include/BS
    include/nlohmann 
    include/miniz
)

# 注意：iconv库现在由UniConv库自动处理

# 源文件
set(LIGHTLOG_SOURCES
    src/log/AsyncRotationManager.cpp
    src/log/BaseLogOutput.cpp
    src/log/BasicLogFormatter.cpp
    src/log/ConsoleLogOutput.cpp
    src/log/FileLogOutput.cpp
    src/log/LightLogWriteImpl.cpp
    src/log/LogCompressor.cpp
    src/log/LogFilters.cpp
    src/log/CompositeFilter.cpp
    src/log/FilterManager.cpp
    src/log/LogOutputManager.cpp
    src/log/MultiOutputLogConfig.cpp
    src/log/RotationErrorHandler.cpp
    src/log/RotationManagerFactory.cpp
    src/log/RotationPreChecker.cpp
    src/log/RotationStateMachine.cpp
    src/log/RotationStrategies.cpp
    src/log/TimeCalculator.cpp
    src/log/TransactionalRotation.cpp
)

# 创建静态库
add_library(lightlog STATIC ${LIGHTLOG_SOURCES})

# 链接UniConv库
target_link_libraries(lightlog PRIVATE UniConv)
target_include_directories(lightlog PRIVATE ${uniconv_SOURCE_DIR}/include)

# 创建主程序
add_executable(lightlog_main main.cpp)
target_link_libraries(lightlog_main lightlog UniConv)
target_include_directories(lightlog_main PRIVATE ${uniconv_SOURCE_DIR}/include)

