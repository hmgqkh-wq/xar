cmake_minimum_required(VERSION 3.10)
project(xeno_wrapper C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Place built shared library directly into the build directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Source files
set(WRAPPER_SOURCES
    src/vulkan_wrapper.c
    src/feature_dump.c
    src/feature_dump_generator.c
)

# Build shared library named "xeno_wrapper"
# CMake will produce -> libxeno_wrapper.so (default "lib" prefix)
add_library(xeno_wrapper SHARED ${WRAPPER_SOURCES})

# Link required Android libraries
target_link_libraries(xeno_wrapper
    log
    m
    atomic
)
