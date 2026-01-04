
add_library(benchmark
   benchmark.hpp
   cputime.hpp
   runner.hpp
   random.hpp
   stopwatch.hpp
   terminal.hpp
   runner.cpp
)

target_compile_options(benchmark PRIVATE -O3 -fno-rtti)
