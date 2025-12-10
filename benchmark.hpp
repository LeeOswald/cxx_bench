#pragma once

#include <chrono>
#include <concepts>
#include <condition_variable>
#include <functional>
#include <iomanip>
#include <mutex>
#include <optional>
#include <ostream>
#include <thread>
#include <vector>


#include <sys/times.h>
#include <time.h>
#include <unistd.h>


namespace Benchmark
{

using Nanos = std::chrono::nanoseconds;


class DefaultTimestampProvider final
{
public:
   using Unit = Nanos;

   constexpr DefaultTimestampProvider() noexcept = default;

   Nanos operator()() const noexcept
   {
      auto delta = Clock::now() - pivot();
      return std::chrono::duration_cast<Nanos>(delta);
   }

private:
   using Clock = std::chrono::high_resolution_clock;
   using TimePoint = Clock::time_point;

   static TimePoint pivot() noexcept
   {
      static TimePoint now = Clock::now();
      return now;
   }
};


class PreciseTimestampProvider final
{
public:
   using Unit = Nanos;

   constexpr PreciseTimestampProvider() noexcept = default;

   Nanos operator()() noexcept
   {
      struct timespec t = {};
      ::clock_gettime(CLOCK_MONOTONIC_RAW, &t);
      std::uint64_t v = t.tv_sec;
      v *= 1000000000ULL;
      v += t.tv_nsec;
      return Nanos{ v };
   }
};


class ThreadCpuTimeProvider final
{
public:
   using Unit = Nanos;

   constexpr ThreadCpuTimeProvider() noexcept = default;

   Nanos operator()() noexcept
   {
      struct timespec t = {};
      ::clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
      std::uint64_t v = t.tv_sec;
      v *= 1000000000ULL;
      v += t.tv_nsec;
      return Nanos{ v };
   }
};


template <class Provider>
class Stopwatch
{
public:
   using Unit = typename Provider::Unit;

   constexpr Stopwatch(Provider pr = {})
      : m_provider(pr)
   {}

   void start() noexcept
   {
      m_started = m_provider();
   }

   Unit stop() noexcept
   {
      auto delta = m_provider() - m_started;
      m_elapsed += delta;
      return delta;
   }

   Unit value() const noexcept
   {
      return m_elapsed;
   }

private:
   Provider m_provider;
   Unit m_started = {};
   Unit m_elapsed = {};
};


class ProcessCpuTimes final
{
public:
   using Duration = std::chrono::milliseconds;

   struct Times
   {
      Duration user;
      Duration system;
   };

   constexpr ProcessCpuTimes() noexcept = default;

   void start() noexcept
   {
      struct tms t = {};
      ::times(&t);

      m_uStart = t.tms_utime;
      m_sStart = t.tms_stime;
   }

   void stop() noexcept
   {
     struct tms t = {};
      ::times(&t);

      m_user += t.tms_utime - m_uStart;
      m_system += t.tms_stime - m_sStart;
   }

   Times value() const noexcept
   {
      return Times {
        std::chrono::milliseconds{m_user * 1000 / m_freq},
        std::chrono::milliseconds{m_system * 1000 / m_freq}
      };
   }

private:
   long m_freq = ::sysconf(_SC_CLK_TCK);
   clock_t m_user = {};
   clock_t m_system = {};
   clock_t m_uStart = {};
   clock_t m_sStart = {};
};


struct Timings final
{
   Nanos threadCpuTime;
   std::optional<ProcessCpuTimes::Times> processCpuTimes;
};


inline Timings run(
   std::invocable auto const& work
)
{
   ProcessCpuTimes pc;
   Stopwatch<ThreadCpuTimeProvider> tc;

   pc.start();
   tc.start();

   work();

   tc.stop();
   pc.stop();

   return Timings {
      tc.value(),
      pc.value()
   };
}


inline Timings run(
   unsigned threads,
   std::invocable auto const& work
)
{
   if (threads < 2)
      return run(work);

   ProcessCpuTimes processCpu;

   std::vector<Stopwatch<ThreadCpuTimeProvider>> threadCpu;
   threadCpu.resize(threads);

   std::vector<std::jthread> workers;
   workers.reserve(threads);

   std::mutex mtx;
   std::condition_variable cv;
   bool start = false;
   std::atomic<unsigned> active = 0;

   for (unsigned id = 0; id < threads; ++id)
   {
      workers.emplace_back(
         [
            id,
            &processCpu,
            &threadCpu,
            &mtx,
            &cv,
            &start,
            &active,
            &work
         ]()
         {
            // start all the workers synchronously
            {
               std::unique_lock l(mtx);
               cv.wait(l, [&start]() { return start; });
            }

            // first stsrted thread starts the global timer
            if (active.fetch_add(1, std::memory_order_acq_rel) == 0)
               processCpu.start();

            threadCpu[id].start();

            work();

            threadCpu[id].stop();

            // the last active thread stops the globsl timer
            if (active.fetch_sub(1, std::memory_order_acq_rel) == 1)
               processCpu.stop();
         }
      );
   }

   // start the workers
   {
      std::lock_guard l(mtx);
      start = true;
   }

   cv.notify_all();

   // wait for the workers
   for (auto& w: workers)
      w.join();


   // average per-thread CPU time
   Nanos averageCpu = {};
   for (auto& cpu : threadCpu)
   {
      averageCpu += cpu.value();
   }

   averageCpu /= threadCpu.size();

   return Timings {
      averageCpu,
      processCpu.value()
   };
}


class Runner
{
public:
   Runner(auto&& name)
      : m_name{std::forward<decltype(name)>(name)}
   {}

   void add(
      auto&& name,
      std::uint64_t iterations,
      unsigned threads,
      std::invocable auto&&  work
   )
   {
      m_bm.emplace_back(
         std::forward<decltype(name)>(name),
         iterations,
         threads,
         std::forward<decltype(work)>(work)
      );
   }

   virtual void run(std::ostream& ss)
   {
      auto line = [&ss]()
      {
         ss << "-------------------‐--------------"
            << "----------------------" << std::endl;
      };

      auto thickLine = [&ss]()
      {
         ss << "≈================================="
            << "======================" << std::endl;
      };

      thickLine();
      ss << m_name << std::endl;
      line();

      if (m_bm.empty())
         return;

      std::size_t id = 0;
      for (auto& bm: m_bm)
      {
         ss << "#" << id++ << ": [" << bm.name << "] ×" << bm.iterations;

         if (bm.threads > 1)
            ss << " ×" << bm.threads << " threads";

         ss << std::endl;

         bm.timings = ::Benchmark::run(
            bm.threads,
            bm.work
         );
      }

      line();
      ss << "  # |"
         << " Total, µs |"
         << " Op, ns |"
         << "    %    |"
         << " CPU (u/s), ms"
         << std::endl;
      line();

      auto best = m_bm.front().timings.threadCpuTime.count();

      id = 0;
      for (auto& bm: m_bm)
      {
         auto du = bm.timings.threadCpuTime.count();
         auto op = du / bm.iterations;
         auto percent = du * 100.0 / double(best);

         ss << std::setw(3) << id++ << " |"
            << std::setw(10) << (du / 1000) <<  " |"
            << std::setw(7) << op << " | "
            << std::setw(7) << std::setprecision(2) << std::fixed
            << percent;

         if (bm.timings.processCpuTimes)
         {
            ss << " | "
               << bm.timings.processCpuTimes->user.count()
               << "/" 
               << bm.timings.processCpuTimes->system.count();
         }

         ss << std::endl;
      }

      line();
   }

private:
   struct Benchmark
   {
      std::string name;
      std::uint64_t iterations;
      unsigned threads;
      std::function<void()> work;
      Timings timings;

      Benchmark(
         std::string_view name,
         std::uint64_t iterations,
         unsigned threads,
         std::function<void()>&& work
      )
         : name(name)
         , iterations(iterations)
         , threads(threads)
         , work(std::move(work))
      {}
   };

   std::string m_name;
   std::vector<Benchmark> m_bm;
};


} // namespace
