#include <benchmark/run.hpp>
#include <benchmark/stopwatch.hpp>
#include <benchmark/timestamp.hpp>


#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>



namespace Benchmark
{


Data run(
   Fixture* f,
   Counter iterations
)
{
   Stopwatch<TimestampProvider> wallTime;
   Stopwatch<ThreadCpuTimeProvider> cpuTime;
   Stopwatch<ThreadCpuUsageProvider> cpuUsage;

   f->initialize(1);

   while (iterations)
   {
      f->prologue(0);

      wallTime.start();
      cpuUsage.start();
      cpuTime.start();

      iterations = f->run(iterations, 0);

      cpuTime.stop();
      cpuUsage.stop();
      wallTime.stop();

      f->epilogue(0);
   }

   f->finalize();

   return Data {
      1,
      wallTime.value(),
      cpuTime.value(),
      cpuUsage.value()
   };
}


Data run(
   unsigned threads,
   Fixture* f,
   Counter iterations
)
{
   if (threads < 2)
      return run(f, iterations);

   Stopwatch<TimestampProvider> wallTime;
   std::vector<Stopwatch<ThreadCpuTimeProvider>> cpuTimes;
   cpuTimes.resize(threads);
   std::vector<Stopwatch<ThreadCpuUsageProvider>> cpuUsages;
   cpuUsages.resize(threads);

   std::vector<std::jthread> workers;
   workers.reserve(threads);

   std::mutex mtx;
   std::condition_variable cv;
   bool start = false;
   std::atomic<unsigned> active = 0;

   f->initialize(threads);

   for (Tid tid = 0; tid < threads; ++tid)
   {
      workers.emplace_back(
         [
            tid,
            &wallTime,
            &cpuTimes,
            &cpuUsages,
            &mtx,
            &cv,
            &start,
            &active,
            f,
            iterations
         ]()
         {
            // start all the workers synchronously
            {
               std::unique_lock l(mtx);
               cv.wait(l, [&start]() { return start; });
            }

            // first started thread starts the global timer
            if (active.fetch_add(1, std::memory_order_acq_rel) == 0)
            {
               wallTime.start();
            }

            auto remaining = iterations;
            while (remaining)
            {
               f->prologue(tid);

               cpuUsages[tid].start();
               cpuTimes[tid].start();

               remaining = f->run(remaining, tid);

               cpuTimes[tid].stop();
               cpuUsages[tid].stop();

               f->epilogue(tid);
            }

            // the last active thread stops the global timer
            if (active.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
               wallTime.stop();
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

   f->finalize();

   decltype(Data::cpuTime) cpuTime = {};
   decltype(Data::cpuUsage) cpuUsage = {};
   for (Tid tid = 0; tid < threads; ++tid)
   {
      cpuTime += cpuTimes[tid].value();
      cpuUsage += cpuUsages[tid].value();
   }

   return Data {
      threads,
      wallTime.value(),
      cpuTime,
      cpuUsage
   };
}


} // namespace
