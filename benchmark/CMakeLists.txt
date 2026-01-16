
add_library(benchmark
   run.cpp
   runner.cpp
)

target_compile_options(benchmark PRIVATE -O3)
