cmake_minimum_required(VERSION 3.25)

project(benchmark VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Export only public symbols
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

if(MSVC)
    add_compile_options("/utf-8")
endif()


add_subdirectory(src)
add_subdirectory(examples)
