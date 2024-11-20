cmake_minimum_required(VERSION 3.13.4)
project(PrintFunctionNamesPass)

set(LLVM_INSTALL_DIR "" CACHE PATH "LLVM installation directory")

list(APPEND CMAKE_PREFIX_PATH "${LLVM_INSTALL_DIR}/lib/cmake/llvm/")

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_C_COMPILER clang)# using gcc is also ok, if you comment this line , than by default gcc will be used on linux.
set(CMAKE_CXX_COMPILER clang++)# using g++ is also ok, if you comment this line , than by default g++ will be used on linux.
set(CMAKE_CXX_STANDARD 23 CACHE STRING "")

if(NOT LLVM_ENABLE_RTTI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

add_library(PrintFunctionNames MODULE PrintFunctionNames.cpp)

target_include_directories(PrintFunctionNames PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
