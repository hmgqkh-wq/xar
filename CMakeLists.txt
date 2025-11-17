cmake_minimum_required(VERSION 3.10)

project(xeno_wrapper C)

set(CMAKE_C_STANDARD 11)

# Enable verbose output
set(CMAKE_VERBOSE_MAKEFILE ON)

# Ensure position-independent code for shared library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Android NDK toolchain location injected by GitHub Actions
if(DEFINED ENV{ANDROID_NDK_HOME})
    set(ANDROID_NDK "$ENV{ANDROID_NDK_HOME}")
else()
    message(FATAL_ERROR "ANDROID_NDK_HOME environment variable not set.")
endif()

# Use the Android toolchain definition
set(CMAKE_TOOLCHAIN_FILE "${ANDROID_NDK}/build/cmake/android.toolchain.cmake")

# Target Android ABI and platform
set(ANDROID_ABI "arm64-v8a")
set(ANDROID_PLATFORM "android-23")

add_library(libxeno_wrapper SHARED
    src/vulkan_wrapper.c
    src/feature_dump.c
    src/feature_dump_generator.c
)

# Libraries required for linking
target_link_libraries(libxeno_wrapper
    log
    m
    atomic
)
