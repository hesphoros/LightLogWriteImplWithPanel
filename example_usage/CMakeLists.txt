# Example: Using LightLog in your CMake project
cmake_minimum_required(VERSION 3.16)
project(LightLogExample VERSION 1.0.0)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent module
include(FetchContent)

# Method 1: Using FetchContent (Recommended)
FetchContent_Declare(
    LightLog
    GIT_REPOSITORY https://github.com/hesphoros/LightLogWriteImpl.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

# Configure LightLog options (optional)
set(LIGHTLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIGHTLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# Make LightLog available
FetchContent_MakeAvailable(LightLog)

# Create executable
add_executable(example_app main.cpp)

# Link LightLog library
target_link_libraries(example_app PRIVATE LightLog::lightlog)

# Set output directory
set_target_properties(example_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Platform-specific settings
if(WIN32)
    if(MSVC)
        target_compile_options(example_app PRIVATE /W4)
    endif()
else()
    target_compile_options(example_app PRIVATE -Wall -Wextra -Wpedantic)
endif()

# =============================================
# Alternative Method 2: Using add_subdirectory
# =============================================
# If you have LightLog as a subdirectory:
# add_subdirectory(../LightLogWriteImpl)
# target_link_libraries(example_app PRIVATE LightLog::lightlog)

# =============================================  
# Alternative Method 3: Using find_package
# =============================================
# If LightLog is installed on the system:
# find_package(LightLog REQUIRED)
# target_link_libraries(example_app PRIVATE LightLog::lightlog)