#pragma once

#include "compiler.hpp"

#include <chrono>

#if BM_POSIX
   #include <sys/resource.h>
#endif


namespace Benchmark
{


#if BM_POSIX

class ThreadCpuTimeProvider final
{
public:
   using Value = std::chrono::nanoseconds;

   constexpr ThreadCpuTimeProvider() noexcept = default;

   Value operator()() noexcept
   {
      struct timespec t = {};
      ::clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
      std::uint64_t v = t.tv_sec;
      v *= 1000000000ULL; // ns
      v += t.tv_nsec;
      return Value{ v };
   }
};

#endif


#if BM_POSIX

class ProcessCpuTimeProvider final
{
public:
   using Value = std::chrono::nanoseconds;

   constexpr ProcessCpuTimeProvider() noexcept = default;

   Value operator()() noexcept
   {
      struct timespec t = {};
      ::clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
      std::uint64_t v = t.tv_sec;
      v *= 1000000000ULL; // ns
      v += t.tv_nsec;
      return Value{ v };
   }
};

#endif


template <typename Unit>
struct CpuUsage
{
   Unit user;
   Unit system;

   constexpr CpuUsage(Unit u = {}, Unit s = {}) noexcept
      : user(u)
      , system(s)
   {}

   void operator+=(const CpuUsage& o) noexcept
   {
      user += o.user;
      system += o.system;
   }

   void operator-=(const CpuUsage& o) noexcept
   {
      user -= o.user;
      system -= o.system;
   }

   friend constexpr CpuUsage operator+(
      const CpuUsage& a,
      const CpuUsage& b
   ) noexcept
   {
      return CpuUsage{
         a.user + b.user,
         a.system + b.system
      };
   }

   friend constexpr CpuUsage operator-(
      const CpuUsage& a,
      const CpuUsage& b
   ) noexcept
   {
      return CpuUsage{
         a.user - b.user,
         a.system - b.system
      };
   }
};


#if BM_POSIX

template <int Who>
class PosixCpuUsageProvider
{
public:
   using Value = CpuUsage<std::chrono::microseconds>;

   constexpr PosixCpuUsageProvider() noexcept = default;

   Value operator()() noexcept
   {
      struct rusage ru = {};
      ::getrusage(Who, &ru);

      std::uint64_t u = ru.ru_utime.tv_sec;
      u *= 1000000ULL; // ms
      u += ru.ru_utime.tv_usec;

      std::uint64_t s = ru.ru_stime.tv_sec;
      s *= 1000000ULL; // ms
      s += ru.ru_stime.tv_usec;

      return Value {
         std::chrono::microseconds(u),
         std::chrono::microseconds(s)
      };
   }
};


using ProcessCpuUsageProvider = PosixCpuUsageProvider<RUSAGE_SELF>;
using ThreadCpuUsageProvider = PosixCpuUsageProvider<RUSAGE_THREAD>;


#endif


} // namespace
