cmake_minimum_required(VERSION 3.25)

project(test VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(atomics atomics.cpp)
target_compile_options(atomics  PRIVATE -O3 -fno-rtti)

add_executable(calls 
   calls.cpp
   calls_impl.cpp
)
target_compile_options(calls PRIVATE -O3 -fno-rtti)

add_executable(rtti rtti.cpp)
target_compile_options(rtti PRIVATE -O3)

add_executable(throw throw.cpp)
target_compile_options(throw PRIVATE -Og -fno-rtti)
target_link_options(throw PRIVATE -rdynamic)
