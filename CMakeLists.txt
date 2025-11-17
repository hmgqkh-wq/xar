cmake_minimum_required(VERSION 3.10)
project(xeno_wrapper C)

# Android / Unix safety flags
set(CMAKE_C_STANDARD 11)
add_definitions(-D_GNU_SOURCE)

# Output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Wrapper source files
set(WRAPPER_SOURCES
    src/vulkan_wrapper.c
    src/feature_dump.c
    src/feature_dump_generator.c
)

# Build shared library with proper name (NO extra "lib" prefix)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
add_library(xeno_wrapper SHARED ${WRAPPER_SOURCES})

# Dependencies
target_link_libraries(xeno_wrapper
    log
    m
    atomic
)
