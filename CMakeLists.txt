# ====================================================================
# LightLogWriteImplWithPanel - 最简CMake配置
# ====================================================================

cmake_minimum_required(VERSION 3.16)

project(LightLogWriteImplWithPanel
    VERSION 1.0.0
    LANGUAGES CXX
)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows 设置
if(WIN32 AND MSVC)
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/utf-8 /wd4996)
endif()

# 包含目录
include_directories(
    include
    include/log
    include/BS
    include/nlohmann 
    include/miniz
)

# iconv 库配置
if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(ICONV_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/lib/iconv/libiconv_1_17d.lib)
    else()
        set(ICONV_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/lib/iconv/libiconv_1_17.lib)
    endif()
endif()

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
    src/log/UniConv.cpp
)

# 创建静态库
add_library(lightlog STATIC ${LIGHTLOG_SOURCES})
if(WIN32)
    target_link_libraries(lightlog PRIVATE ${ICONV_LIBRARIES})
endif()

# 创建主程序
add_executable(lightlog_main main.cpp)
target_link_libraries(lightlog_main lightlog)

