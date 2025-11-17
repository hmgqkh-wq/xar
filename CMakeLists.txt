cmake_minimum_required(VERSION 3.10)
project(xeno_vulkan_wrapper C)

# Build a shared library named exactly libvulkan.xeno_wrapper.so
add_library(vulkan_xeno_wrapper SHARED
    src/vulkan_wrapper.c
    src/feature_dump.c
)

# We want the .so file to be called libvulkan.xeno_wrapper.so
set_target_properties(vulkan_xeno_wrapper PROPERTIES
    OUTPUT_NAME "vulkan.xeno_wrapper"
)

# Ensure PIC and proper flags
target_compile_options(vulkan_xeno_wrapper PRIVATE -fPIC -O2 -Wall)
target_include_directories(vulkan_xeno_wrapper PRIVATE include)

# For Android build flavours you may need: set(ANDROID TRUE) and configure toolchain externally
