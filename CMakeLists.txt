cmake_minimum_required(VERSION 3.10)
project(xclipse_forward_wrapper C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_definitions(_GNU_SOURCE)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(vulkan_wrapper SHARED src/forward_wrapper.c)
set_target_properties(vulkan_wrapper PROPERTIES OUTPUT_NAME "libvulkan")

find_package(Threads REQUIRED)
target_link_libraries(vulkan_wrapper m dl pthread)
