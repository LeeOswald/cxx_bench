#pragma once

#include <chrono>

#if BM_POSIX
   #include <time.h>
#endif


namespace Benchmark
{


class DefaultTimestampProvider final
{
public:
   using Unit = std::chrono::nanoseconds;

   constexpr DefaultTimestampProvider() noexcept = default;

   Unit operator()() const noexcept
   {
      auto delta = std::chrono::steady_clock::now().time_since_epoch();
      return std::chrono::duration_cast<Unit>(delta);
   }
};


#if BM_POSIX

class PosixTimestampProvider final
{
public:
   using Unit = std::chrono::nanoseconds;

   constexpr PosixTimestampProvider() noexcept = default;

   Unit operator()() noexcept
   {
      struct timespec t = {};
      ::clock_gettime(CLOCK_MONOTONIC_RAW, &t);
      std::uint64_t v = t.tv_sec;
      v *= 1000000000ULL;
      v += t.tv_nsec;
      return Unit{ v };
   }
};


#endif


} // namespace
