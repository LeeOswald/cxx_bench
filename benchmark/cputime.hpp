#pragma once

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

#if BM_POSIX

template <int Who>
class PosixCpuUsageProvider
{
public:
   using Unit = std::chrono::microseconds;

   struct Times
   {
      Unit user;
      Unit system;
   };

   constexpr PosixCpuUsageProvider() noexcept = default;

   void start() noexcept
   {
      struct rusage u = {};
      ::getrusage(Who, &u);

      m_uStart = u.ru_utime.tv_sec;
      m_uStart *= 1000000ULL;
      m_uStart += u.ru_utime.tv_usec;

      m_sStart = u.ru_stime.tv_sec;
      m_sStart *= 1000000ULL;
      m_sStart += u.ru_stime.tv_usec;
   }

   void stop() noexcept
   {
      struct rusage u = {};
      ::getrusage(Who, &u);

      std::uint64_t uEnd = u.ru_utime.tv_sec;
      uEnd *= 1000000ULL;
      uEnd += u.ru_utime.tv_usec;

      std::uint64_t sEnd = u.ru_stime.tv_sec;
      sEnd *= 1000000ULL;
      sEnd += u.ru_stime.tv_usec;

      m_user += (uEnd - m_uStart);
      m_system += (sEnd - m_sStart);
   }

   Times value() const noexcept
   {
      return Times {
         Unit{m_user},
         Unit{m_system}
      };
   }

private:
   std::uint64_t m_user = {};
   std::uint64_t m_system = {};
   std::uint64_t m_uStart = {};
   std::uint64_t m_sStart = {};
};


using ProcessCpuUsageProvider = PosixCpuUsageProvider<RUSAGE_SELF>;
using ThreadCpuUsageProvider = PosixCpuUsageProvider<RUSAGE_THREAD>;


#endif


} // namespace
