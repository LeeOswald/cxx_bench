
add_library(benchmark
   benchmark.cpp
   runner.cpp
)

target_compile_options(benchmark PRIVATE -O3 -fno-rtti)
