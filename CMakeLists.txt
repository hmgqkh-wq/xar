cmake_minimum_required(VERSION 3.10)
project(xclipse_wrapper C)

set(CMAKE_C_STANDARD 11)

# Android build flags (required for shared library)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Include headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Build Vulkan wrapper shared library
add_library(xeno_wrapper SHARED
    src/vulkan_wrapper.c
    src/feature_dump.c
)

# Link log library (Android)
target_link_libraries(xeno_wrapper log)

# Output name (libxeno_wrapper.so)
set_target_properties(xeno_wrapper PROPERTIES
    OUTPUT_NAME "xeno_wrapper"
)
