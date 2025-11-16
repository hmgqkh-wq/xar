cmake_minimum_required(VERSION 3.16)
project(xclipse_wrapper C)

set(CMAKE_C_STANDARD 11)

add_compile_definitions(
    XCLIPSE_ENABLE_LOGGING
    XCLIPSE_ENABLE_FEATURE_DUMP
    XCLIPSE_ENABLE_PIPELINE_VALIDATION
    XCLIPSE_ENABLE_BC_FALLBACK
)

include_directories(include)

# Wrapper library (the Vulkan layer replacement)
add_library(xeno_wrapper SHARED
    src/xeno_wrapper.c
)

# Feature dump tool (runs on device or in action)
add_executable(xclipse_feature_dump_generator
    src/xeno_feature_dump_generator.c
)

target_link_libraries(xclipse_feature_dump_generator m pthread)
