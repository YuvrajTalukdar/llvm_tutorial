cmake_minimum_required(VERSION 3.10)
project(ParseCudaDirectives_LibTooling)

set(CMAKE_C_COMPILER clang)# using gcc is also ok, if you comment this line , than by default gcc will be used on linux.
set(CMAKE_CXX_COMPILER clang++)# using g++ is also ok, if you comment this line , than by default g++ will be used on linux.

find_package(Clang REQUIRED CONFIG)
find_package(LLVM REQUIRED CONFIG)

include_directories(${Clang_INCLUDE_DIRS})
include_directories(${LLVM_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})

add_executable(FunctionPrinter main.cpp)

# Link against `libclang-cpp` and `libLLVM`
#link_directories(/opt/cuda/lib64)
target_link_libraries(FunctionPrinter PRIVATE clang-cpp LLVM /opt/cuda/lib64/libcudart.so)

#target_link_libraries(FunctionPrinter PRIVATE ${/usr/lib/libclang.so.18.1.8} LLVM)
