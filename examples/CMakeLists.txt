
add_executable(counter counter.cpp)
target_compile_options(counter PRIVATE -O3 -fno-rtti)
target_link_libraries(counter PRIVATE benchmark)

add_executable(calls calls.cpp)
target_compile_options(calls PRIVATE -O3 -fno-rtti)
target_link_libraries(calls PRIVATE benchmark)

add_executable(malloc malloc.cpp)
target_compile_options(malloc PRIVATE -O3 -fno-rtti)
target_link_libraries(malloc PRIVATE benchmark)

add_executable(rtti rtti.cpp)
target_compile_options(rtti PRIVATE -O3)
target_link_libraries(rtti PRIVATE benchmark)

add_executable(throw throw.cpp)
target_compile_options(throw PRIVATE -Og -fno-rtti)
target_link_options(throw PRIVATE -rdynamic)
target_link_libraries(throw PRIVATE benchmark)

