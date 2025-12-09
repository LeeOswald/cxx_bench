cmake_minimum_required(VERSION 3.25)

project(test VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(atomics atomics.cpp)
target_compile_options(atomics  PRIVATE -O3)

add_executable(calls calls.cpp)
target_compile_options(calls PRIVATE -O3)
