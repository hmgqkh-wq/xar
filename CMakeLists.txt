cmake_minimum_required(VERSION 3.10)
project(xeno_wrapper C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -Wno-unused-parameter")

# We will produce a library named libvulkan.xeno_wrapper.so (Eden expected prefix)
set(LIB_NAME vulkan.xeno_wrapper)

add_library(${LIB_NAME} SHARED
    src/vulkan_wrapper.c
    src/feature_dump.c
)

# Link with dl for dlsym, pthread for thread-safety (logging)
find_library(LIB_DL dl REQUIRED)
find_package(Threads REQUIRED)

target_link_libraries(${LIB_NAME} PRIVATE ${LIB_DL} Threads::Threads)

# Set the SONAME to match library file
set_target_properties(${LIB_NAME} PROPERTIES
    OUTPUT_NAME "libvulkan.xeno_wrapper"
    PREFIX ""
)

# Install path for packaging into the wrapper zip (not used by CI directly).
install(TARGETS ${LIB_NAME}
        LIBRARY DESTINATION lib/arm64-v8a)
