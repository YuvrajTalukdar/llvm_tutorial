cmake_minimum_required(VERSION 3.20)
project(ParseCudaDirectives_LibTooling)


# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# Available build types
set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING "Build type" FORCE)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Add debug-specific definitions
    add_definitions(-DDEBUG_MODE)

    # Enable debug symbols and turn off optimizations
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
else()
    add_definitions(-DRELEASE_MODE)

    # Enable debug symbols and turn off optimizations
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O2")
    set(CMAKE_C_FLAGS_DEBUG "-O2")
endif()



set(CMAKE_C_COMPILER clang)# using gcc is also ok, if you comment this line , than by default gcc will be used on linux.
set(CMAKE_CXX_COMPILER clang++)# using g++ is also ok, if you comment this line , than by default g++ will be used on linux.

find_package(Clang REQUIRED CONFIG)
find_package(LLVM REQUIRED CONFIG)

include_directories(${Clang_INCLUDE_DIRS})
include_directories(${LLVM_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})

add_executable(matcher matcher.cpp)

# Link against `libclang-cpp` and `libLLVM`
target_link_libraries(matcher PRIVATE clang-cpp LLVM /opt/cuda/lib64/libcudart.so)


