cmake_minimum_required(VERSION 3.25)

project(benchmark VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(-DBM_64=1)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_definitions(-DBM_32=1)
else()
    message(FATAL_ERROR "Unsupported bitness")
endif()

# Export only public symbols
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# detect OS
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0602)
    add_definitions(-DBM_WINDOWS=1)
elseif(ANDROID)
    add_definitions(-DBM_POSIX=1 -DBM_ANDROID=1)
else()
    message(FATAL_ERROR "Unsupported OS")
endif()

if(MSVC)
    add_compile_options("/utf-8")
endif()


add_subdirectory(benchmark)

add_executable(ref_count ref_count.cpp)
target_compile_options(ref_count PRIVATE -O3 -fno-rtti)
target_link_libraries(ref_count PRIVATE benchmark)

add_executable(calls calls.cpp)
target_compile_options(calls PRIVATE -O3 -fno-rtti)
target_link_libraries(calls PRIVATE benchmark)

add_executable(rtti rtti.cpp)
target_compile_options(rtti PRIVATE -O3)
target_link_libraries(rtti PRIVATE benchmark)

add_executable(throw throw.cpp)
target_compile_options(throw PRIVATE -Og -fno-rtti)
target_link_options(throw PRIVATE -rdynamic)
target_link_libraries(throw PRIVATE benchmark)

