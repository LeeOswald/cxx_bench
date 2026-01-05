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
   using Value = std::chrono::nanoseconds;

   constexpr DefaultTimestampProvider() noexcept = default;

   Value operator()() const noexcept
   {
      using Clock = std::chrono::steady_clock;
      auto d = Clock::now().time_since_epoch();
      return std::chrono::duration_cast<Value>(d);
   }
};


#if BM_POSIX

class PosixTimestampProvider final
{
public:
   using Value = std::chrono::nanoseconds;

   constexpr PosixTimestampProvider() noexcept = default;

   Value operator()() noexcept
   {
      struct timespec t = {};
      ::clock_gettime(CLOCK_MONOTONIC_RAW, &t);
      std::uint64_t v = t.tv_sec;
      v *= 1000000000ULL; // ns
      v += t.tv_nsec;
      return Value{ v };
   }
};


#endif


#if BM_POSIX
using TimestampProvider = PosixTimestampProvider;
#else
using TimestampProvider = DefaultTimestampProvider;
#endif


} // namespace
