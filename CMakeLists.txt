cmake_minimum_required(VERSION 3.10)
project(xeno_wrapper C)

# Ensure position independent code for shared library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Provide an option to control ABI if CI sets it
if(NOT ANDROID_ABI)
  set(ANDROID_ABI "arm64-v8a" CACHE STRING "Target Android ABI")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files (explicit)
set(SRC_FILES
    src/vulkan_wrapper.c
    src/feature_dump.c
    src/feature_dump_generator.c
)

# Output name must be libvulkan.xeno_wrapper.so (Eden expects libvulkan.* prefix)
add_library(vulkan.xeno_wrapper SHARED ${SRC_FILES})

# Set the soname / output library name to libvulkan.xeno_wrapper.so
set_target_properties(vulkan.xeno_wrapper PROPERTIES
    OUTPUT_NAME "vulkan.xeno_wrapper"
    PREFIX "lib"
)

# Compile flags: keep conservative for cross-compile
target_compile_options(vulkan.xeno_wrapper PRIVATE -Wall -Wextra)
target_compile_features(vulkan.xeno_wrapper PRIVATE c_std_11)

# When building for Android NDK, typical toolchain will provide necessary flags.
# No external libraries required; linking is minimal.
