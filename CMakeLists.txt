cmake_minimum_required(VERSION 3.10)
project(xclipse_superwrapper C)

set(CMAKE_C_STANDARD 11)

add_library(xeno_wrapper SHARED
    src/xeno_wrapper.c
    src/bc_emulate.c
)

add_executable(xclipse_feature_dump_generator
    src/xeno_feature_dump_generator.c
)

find_package(Threads REQUIRED)
target_link_libraries(xeno_wrapper Threads::Threads)
target_link_libraries(xclipse_feature_dump_generator Threads::Threads)

set_target_properties(xeno_wrapper PROPERTIES OUTPUT_NAME "libxeno_wrapper")
