# LightLog Example Usage

This directory contains a complete example showing how to use LightLog in your own CMake project.

## Files

- `CMakeLists.txt` - CMake configuration showing three integration methods
- `main.cpp` - Example application demonstrating key features
- `README.md` - This file

## How to Build and Run

### Method 1: FetchContent (from this directory)

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
./bin/example_app  # Linux/macOS
# or
./bin/Release/example_app.exe  # Windows
```

### Method 2: Using LightLog as subdirectory

If you have LightLog source code locally:

```cmake
# Update CMakeLists.txt to uncomment the subdirectory method
add_subdirectory(../path/to/LightLogWriteImpl)
target_link_libraries(example_app PRIVATE LightLog::lightlog)
```

### Method 3: Using installed LightLog

If you have installed LightLog on your system:

```cmake
# Update CMakeLists.txt to uncomment the find_package method
find_package(LightLog REQUIRED)
target_link_libraries(example_app PRIVATE LightLog::lightlog)
```

## What the Example Demonstrates

1. **Basic Setup**: Creating logger with compressor
2. **Configuration**: Setting up log files and rotation
3. **Multi-Output**: Console and file outputs simultaneously
4. **Filtering**: Level-based filtering
5. **Callbacks**: Event-driven log processing
6. **Rotation**: Manual log rotation
7. **Statistics**: Compression and performance metrics
8. **Cleanup**: Proper resource management

## Expected Output

The example will:
- Create a `logs/` directory
- Write filtered messages to console and files
- Demonstrate log rotation and compression
- Show callback system working
- Display compression statistics

## Integration Patterns

This example shows the recommended patterns for:
- CMake integration
- Logger initialization
- Configuration management
- Resource cleanup
- Error handling

You can use this as a template for your own applications.