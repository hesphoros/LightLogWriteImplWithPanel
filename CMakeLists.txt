# ====================================================================
# LightLogWriteImplWithPanel - Modern CMake Configuration
# ====================================================================

cmake_minimum_required(VERSION 3.16)

# 检查是否作为子项目被包含
set(LIGHTLOG_IS_MAIN_PROJECT FALSE)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(LIGHTLOG_IS_MAIN_PROJECT TRUE)
endif()

# 项目声明
project(LightLogWriteImplWithPanel
    VERSION 1.0.0
    DESCRIPTION "Modern C++17 Logging Library with Advanced Features"
    HOMEPAGE_URL "https://github.com/hesphoros/LightLogWriteImplWithPanel"
    LANGUAGES CXX
)

# 选项配置
option(LIGHTLOG_BUILD_EXAMPLES "Build example programs" ${LIGHTLOG_IS_MAIN_PROJECT})
option(LIGHTLOG_BUILD_TESTS "Build test programs" ${LIGHTLOG_IS_MAIN_PROJECT})
option(LIGHTLOG_INSTALL "Generate installation target" ${LIGHTLOG_IS_MAIN_PROJECT})
option(LIGHTLOG_BUILD_SHARED "Build shared library" OFF)

# C++标准设置
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译器特定设置
if(WIN32 AND MSVC)
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/utf-8 /wd4996)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# FetchContent模块
include(FetchContent)

# UniConv依赖
FetchContent_Declare(
    UniConv
    GIT_REPOSITORY https://github.com/hesphoros/UniConv.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(UniConv)

# 源文件列表
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
    src/log/LogFilterFactory.cpp
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

# 头文件列表
set(LIGHTLOG_HEADERS
    include/log/AsyncRotationManager.h
    include/log/BaseLogOutput.h
    include/log/BasicLogFormatter.h
    include/log/CompositeFilter.h
    include/log/ConsoleLogOutput.h
    include/log/DebugUtils.h
    include/log/FileLogOutput.h
    include/log/FilterManager.h
    include/log/ICompositeFilter.h
    include/log/IFilterManager.h
    include/log/ILogCompressor.h
    include/log/ILogFilter.h
    include/log/ILogFormatter.h
    include/log/ILogOutput.h
    include/log/ILogRotationManager.h
    include/log/IRotationStrategy.h
    include/log/LightLogWriteImpl.h
    include/log/LogCommon.h
    include/log/LogCompressor.h
    include/log/LogFilterFactory.h
    include/log/LogFilters.h
    include/log/LogOutputManager.h
    include/log/LogRotationManager.h
    include/log/MultiOutputLogConfig.h
    include/log/PerformanceProfiler.h
    include/log/RotationErrorHandler.h
    include/log/RotationManagerFactory.h
    include/log/RotationPreChecker.h
    include/log/RotationStateMachine.h
    include/log/RotationStrategies.h
    include/log/singleton.h
    include/log/TimeCalculator.h
    include/log/TransactionalRotation.h
    include/log/UniConvAdapter.h
)

# 创建库目标
if(LIGHTLOG_BUILD_SHARED)
    add_library(lightlog SHARED ${LIGHTLOG_SOURCES} ${LIGHTLOG_HEADERS})
    target_compile_definitions(lightlog PRIVATE LIGHTLOG_BUILD_SHARED)
    set_target_properties(lightlog PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
else()
    add_library(lightlog STATIC ${LIGHTLOG_SOURCES} ${LIGHTLOG_HEADERS})
endif()

# 创建别名目标
add_library(LightLog::lightlog ALIAS lightlog)

# 目标属性设置
target_compile_features(lightlog PUBLIC cxx_std_17)

# 包含目录
target_include_directories(lightlog
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/BS>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nlohmann>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/miniz>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 链接依赖
target_link_libraries(lightlog 
    PUBLIC 
        UniConv
    PRIVATE
        $<$<PLATFORM_ID:Windows>:ws2_32>
)

# 预处理器定义
target_compile_definitions(lightlog
    PUBLIC
        $<$<CONFIG:Debug>:LIGHTLOG_DEBUG>
    PRIVATE
        LIGHTLOG_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        LIGHTLOG_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        LIGHTLOG_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# 示例程序
if(LIGHTLOG_BUILD_EXAMPLES AND LIGHTLOG_IS_MAIN_PROJECT)
    # 主要演示程序
    add_executable(lightlog_demo examples/demo_main.cpp)
    target_link_libraries(lightlog_demo PRIVATE LightLog::lightlog)
    set_target_properties(lightlog_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    
    # 简单演示程序
    add_executable(simple_demo examples/simple_demo.cpp)
    target_link_libraries(simple_demo PRIVATE LightLog::lightlog)
    set_target_properties(simple_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
    )
endif()

# 测试程序
if(LIGHTLOG_BUILD_TESTS AND LIGHTLOG_IS_MAIN_PROJECT)
    enable_testing()
    file(GLOB TEST_FILES "test/*.cpp")
    foreach(TEST_FILE ${TEST_FILES})
        get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_FILE})
        target_link_libraries(${TEST_NAME} PRIVATE LightLog::lightlog)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
        set_target_properties(${TEST_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
        )
    endforeach()
endif()

# 安装配置
if(LIGHTLOG_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    
    # 安装目标
    install(TARGETS lightlog
        EXPORT LightLogTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    
    # 如果UniConv是INTERFACE库，也要安装它
    if(TARGET UniConv)
        install(TARGETS UniConv
            EXPORT LightLogTargets
        )
    endif()
    
    # 安装头文件
    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    
    # 生成并安装目标配置文件
    install(EXPORT LightLogTargets
        FILE LightLogTargets.cmake
        NAMESPACE LightLog::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LightLog
    )
    
    # 配置包配置文件
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LightLogConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/LightLogConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LightLog
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR
    )
    
    # 生成版本文件
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/LightLogConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    
    # 安装配置文件
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/LightLogConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/LightLogConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LightLog
    )
    
    # 安装配置文件和示例
    install(DIRECTORY config/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/LightLog/config
        FILES_MATCHING PATTERN "*.json"
    )
    
    # 安装文档
    install(DIRECTORY docs/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
        FILES_MATCHING PATTERN "*.md"
    )
endif()

# 包配置
set(CPACK_PACKAGE_NAME "LightLog")
set(CPACK_PACKAGE_VENDOR "hesphoros")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern C++17 Logging Library")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
endif()

include(CPack)

