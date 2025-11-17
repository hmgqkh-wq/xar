cmake_minimum_required(VERSION 3.10)
project(xclipse_wrapper C)

# Use C standard that works on Android
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build a shared lib named "vulkan.xeno_wrapper" -> libvulkan.xeno_wrapper.so
set(SHARED_LIB_NAME "vulkan.xeno_wrapper")

# Set default output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Source files
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(SOURCES
    ${SRC_DIR}/vulkan_wrapper.c
    ${SRC_DIR}/feature_dump.c
    ${SRC_DIR}/feature_dump_generator.c
)

add_library(${SHARED_LIB_NAME} SHARED ${SOURCES})

# Ensure standard android link libraries
target_link_libraries(${SHARED_LIB_NAME}
    log
    m
    atomic
    dl
)

# Make sure position independent code is enabled
set_property(TARGET ${SHARED_LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
