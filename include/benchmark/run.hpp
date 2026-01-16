#pragma once


#include <benchmark/cputime.hpp>
#include <benchmark/fixture.hpp>

#include <chrono>
#include <concepts>
#include <functional>


namespace Benchmark
{



struct Data
{
   unsigned threads;
   std::chrono::nanoseconds wallTime;
   std::chrono::nanoseconds cpuTime;
   CpuUsage<std::chrono::microseconds> cpuUsage;
};


Data run(Fixture* f, Counter iterations);

Data run(unsigned threads, Fixture* f, Counter iterations);


} // namespace
