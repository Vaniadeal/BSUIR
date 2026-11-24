cmake_minimum_required(VERSION 3.16)
project(linked_list_filesystem VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization and warning flags
add_compile_options(-Wall -Wextra -Wpedantic -O2)

# Include directories
include_directories(include)

# Filesystem Core Library
add_library(fs_lib STATIC
    src/block_device.cpp
    src/file_system.cpp
    src/cli.cpp
)

# Main CLI Application
add_executable(fs_app src/main.cpp)
target_link_libraries(fs_app PRIVATE fs_lib)

# Unit Tests Executable
add_executable(test_fs tests/test_fs.cpp)
target_link_libraries(test_fs PRIVATE fs_lib)

# Benchmark Executable
add_executable(benchmark_app tests/benchmark.cpp)
target_link_libraries(benchmark_app PRIVATE fs_lib)

# Enable CTest
enable_testing()
add_test(NAME CoreFileSystemTests COMMAND test_fs)
