#pragma once

#include "cputime.hpp"
#include "timestamp.hpp"

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


#include <sys/resource.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>


#define BM_DONT_OPTIMIZE \
   __attribute__((noinline)) __attribute__((optnone))


namespace Benchmark
{


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
   using Unit = std::chrono::microseconds;

   struct Times
   {
      Unit user;
      Unit system;
   };

   constexpr ProcessCpuTimes() noexcept = default;

   void start() noexcept
   {
      struct rusage u = {};
      ::getrusage(RUSAGE_SELF, &u);

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
      ::getrusage(RUSAGE_SELF, &u);

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


struct Timings final
{
   std::chrono::nanoseconds threadCpuTime;
   std::optional<ProcessCpuTimes::Times> processCpuTimes;
};


template <typename T>
concept BenchmarkItem =
   std::is_invocable_v<T, std::uint64_t>;


inline Timings run(
   BenchmarkItem auto const& work,
   std::uint64_t iterations
)
{
   ProcessCpuTimes pc;
   Stopwatch<ThreadCpuTimeProvider> tc;

   pc.start();
   tc.start();

   work(iterations);

   tc.stop();
   pc.stop();

   return Timings {
      tc.value(),
      pc.value()
   };
}


inline Timings run(
   unsigned threads,
   BenchmarkItem  auto const& work,
   std::uint64_t iterations
)
{
   if (threads < 2)
      return run(work, iterations);

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
            &work,
            iterations
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

            work(iterations);

            threadCpu[id].stop();

            // the last active thread stops the global timer
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
   std::chrono::nanoseconds averageCpu = {};
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
   Runner(auto&& name, std::uint64_t iterations)
      : m_name{std::forward<decltype(name)>(name)}
      , m_iterations(iterations)
   {}

   void add(
      auto&& name,
      unsigned threads,
      BenchmarkItem auto&& work
   )
   {
      m_bm.emplace_back(
         std::forward<decltype(name)>(name),
         threads,
         std::forward<decltype(work)>(work),
         true
      );
   }

   void add(
      unsigned threads,
      BenchmarkItem auto&& work
   )
   {
      m_bm.emplace_back(
         "",
         threads,
         std::forward<decltype(work)>(work),
         false
      );
   }
   virtual void run(std::ostream& ss)
   {
      auto line = [&ss]()
      {
         ss << "-------------------‐--------------"
            << "----------------------" << std::endl;
      };

      auto sub_line = [&ss]()
      {
         ss << "   ----------------‐--------------"
            << "----------------------" << std::endl;
      };

      auto thickLine = [&ss]()
      {
         ss << "≈================================="
            << "======================" << std::endl;
      };

      thickLine();
      ss << m_name;
      if (m_iterations > 0)
         ss << " (" << m_iterations << " iterations)";

      ss <<  std::endl;
      line();

      if (m_bm.empty())
         return;

      std::string name;
      std::size_t index = 0;

      for (auto& bm: m_bm)
      {
         if (bm.group)
            name = bm.name;

         ss << "#" << (index + 1) << " / " << m_bm.size()
            << ": " << name;

         if (bm.threads > 1)
            ss << " ×" << bm.threads << " threads";

         ss << std::endl;

         bm.timings = ::Benchmark::run(
            bm.threads,
            bm.work,
            m_iterations
         );

         ++index;
      }

      line();
      ss << " × |"
         << " Total, µs |"
         << " Op, ns |"
         << "    %    |"
         << " CPU (u/s), ms"
         << std::endl;

      auto best = m_bm.front().timings.threadCpuTime.count();

      std::size_t id = 0;
      for (auto& bm: m_bm)
      {
         if (id == 0 || bm.group)
            line();

         if (bm.group)
         {
            ss << " " << bm.name << std::endl;
            sub_line();
         }

         auto du = bm.timings.threadCpuTime.count();
         auto op = du / m_iterations;
         auto percent = du * 100.0 / double(best);

         ss << std::setw(2) << bm.threads << " |"
            << std::setw(10) << (du / 1000) <<  " |"
            << std::setw(7) << op << " | ";

         if (percent < 200)
         {
            ss << std::setw(7) << std::setprecision(2) << std::fixed
               << percent;
         }
         else
         {
            ss << std::setw(7)
               << unsigned(percent);
         }

         if (bm.timings.processCpuTimes)
         {
            auto u =
               bm.timings.processCpuTimes->user.count() / 1000;

            ss << " | " << u;

            auto s =
               bm.timings.processCpuTimes->system.count() / 1000;

            if (s > 0)
               ss << " / " << s;
         }

         ss << std::endl;
         ++id;
      }

      line();
   }

private:
   struct Benchmark
   {
      std::string name;
      unsigned threads;
      std::function<void(std::uint64_t)> work;
      bool group;
      Timings timings;

      Benchmark(
         std::string_view name,
         unsigned threads,
         std::function<void(std::uint64_t)>&& work,
         bool group
      )
         : name(name)
         , threads(threads)
         , work(std::move(work))
         , group(group)
      {}
   };

   std::string m_name;
   std::uint64_t m_iterations;
   std::vector<Benchmark> m_bm;
};


} // namespace
