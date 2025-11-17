cmake_minimum_required(VERSION 3.10)

project(xeno_wrapper LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(libxeno_wrapper SHARED
    src/vulkan_wrapper.c
    src/feature_dump.c
    src/feature_dump_generator.c
)

# Android does NOT support -lpthread
# Android uses libc (bionic) which already has pthread symbols
target_link_libraries(libxeno_wrapper
    log        # Android logging
    m          # Math library
)
