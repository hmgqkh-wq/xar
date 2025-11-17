cmake_minimum_required(VERSION 3.10)
project(xeno_wrapper C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build shared library without "lib" prefix
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")

# Android ABI
if(NOT DEFINED ANDROID_PLATFORM)
    set(ANDROID_PLATFORM android-23)
endif()

message(STATUS "Building for Android: ABI=${ANDROID_ABI} PLATFORM=${ANDROID_PLATFORM}")

add_library(xeno_wrapper SHARED
    src/vulkan_wrapper.c
    src/feature_dump.c
)

target_link_libraries(xeno_wrapper
    log
    android
)
