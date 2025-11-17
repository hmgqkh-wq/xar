cmake_minimum_required(VERSION 3.10)
project(xclipse_wrapper C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_definitions(_GNU_SOURCE)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(xeno_wrapper SHARED src/vulkan_wrapper.c src/feature_dump.c)
set_target_properties(xeno_wrapper PROPERTIES OUTPUT_NAME "libxeno_wrapper")

find_package(Threads REQUIRED)
target_link_libraries(xeno_wrapper m dl pthread)
