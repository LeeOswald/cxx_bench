#pragma once

#include <chrono>

#if BM_POSIX
   #include <time.h>
#endif


namespace Benchmark
{


#if BM_POSIX

class ThreadCpuTimeProvider final
{
public:
   using Unit = std::chrono::nanoseconds;

   constexpr ThreadCpuTimeProvider() noexcept = default;

   Unit operator()() noexcept
   {
      struct timespec t = {};
      ::clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
      std::uint64_t v = t.tv_sec;
      v *= 1000000000ULL;
      v += t.tv_nsec;
      return Unit{ v };
   }
};

#endif


#if BM_POSIX

class ProcessCpuTimeProvider final
{
public:
   using Unit = std::chrono::nanoseconds;

   constexpr ProcessCpuTimeProvider() noexcept = default;

   Unit operator()() noexcept
   {
      struct timespec t = {};
      ::clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
      std::uint64_t v = t.tv_sec;
      v *= 1000000000ULL;
      v += t.tv_nsec;
      return Unit{ v };
   }
};

#endif

} // namespace
