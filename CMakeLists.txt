cmake_minimum_required(VERSION 3.10)
project(xeno_wrapper C)

# Optionally allow cross-compiling by passing -DANDROID_ABI=arm64-v8a -DANDROID_NDK=/path/to/ndk
if(DEFINED ANDROID_NDK)
  set(CMAKE_SYSTEM_NAME Android)
  set(CMAKE_ANDROID_NDK ${ANDROID_NDK})
  if(NOT DEFINED CMAKE_ANDROID_ARCH_ABI)
    set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
  endif()
  # set API if not provided
  if(NOT DEFINED ANDROID_PLATFORM)
    set(ANDROID_PLATFORM 23)
  endif()
  set(CMAKE_ANDROID_API ${ANDROID_PLATFORM})
endif()

# Build type default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Compiler flags (portable)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -fvisibility=hidden)
endif()

# Build wrapper shared library
file(GLOB SRC_FILES "src/*.c" "src/*.h")
add_library(xeno_wrapper SHARED
  src/vulkan_wrapper.c
)

# Expose a predictable SONAME
set_target_properties(xeno_wrapper PROPERTIES
  OUTPUT_NAME "xeno_wrapper"
  VERSION 1.0.0
)

# Linker settings (for Android: many libs are provided by libc++/system)
target_link_libraries(xeno_wrapper
  PRIVATE
    m
)

# Build a tiny generator/tool that the workflow runs to create feature dumps
add_executable(xeno_feature_dump_generator src/feature_dump_generator.c)
target_compile_definitions(xeno_feature_dump_generator PRIVATE FEATURE_DUMP_TOOL)

# Install layout for packaging
install(TARGETS xeno_wrapper
  LIBRARY DESTINATION lib/arm64-v8a
)

install(TARGETS xeno_feature_dump_generator
  RUNTIME DESTINATION bin
)

# Add an option to embed manifest into the package (no action here — manifest should be top-level in zip)
