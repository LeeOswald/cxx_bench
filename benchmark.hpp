#pragma once

#include "cputime.hpp"
#include "stopwatch.hpp"
#include "timestamp.hpp"

#include <chrono>
#include <concepts>
#include <condition_variable>
#include <functional>
#include <iomanip>
#include <mutex>
#include <thread>
#include <vector>


#define BM_NOINLINE \
   __attribute__((noinline))

#define BM_DONT_OPTIMIZE \
  __attribute__((optnone))


namespace Benchmark
{


struct Timings final
{
   using Stopwatch = Stopwatch<TimestampProvider>;
   using CpuUsageProvider = ProcessCpuUsageProvider;

   Stopwatch::Unit time;
   CpuUsageProvider::Times cpu;
};


template <typename T>
concept BenchmarkItem =
   std::is_invocable_v<T, std::uint64_t>;


inline Timings run(
   BenchmarkItem auto const& work,
   std::uint64_t iterations
)
{
   Timings::CpuUsageProvider cpu;
   Timings::Stopwatch sw;

   cpu.start();
   sw.start();

   work(iterations);

   sw.stop();
   cpu.stop();

   return Timings {
      sw.value(),
      cpu.value()
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

   Timings::CpuUsageProvider cpu;
   Timings::Stopwatch sw;

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
            &cpu,
            &sw,
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
            {
               cpu.start();
               sw.start();
            }

            work(iterations);

            // the last active thread stops the global timer
            if (active.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
               sw.stop();
               cpu.stop();
            }
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


   return Timings {
      sw.value(),
      cpu.value()
   };
}


} // namespace
