#pragma once

#include <chrono>
#include <concepts>
#include <condition_variable>
#include <functional>
#include <iomanip>
#include <mutex>
#include <ostream>
#include <thread>
#include <vector>


namespace Benchmark
{

class Stopwatch final
{
public:
   using Clock = std::chrono::high_resolution_clock;
   using Duration = Clock::duration;

   Stopwatch() noexcept = default;

   void start() noexcept
   {
      m_started = Clock::now();
   }

   auto stop() noexcept
   {
      auto delta = Clock::now() - m_started;
      m_elapsed += delta;
      return delta;
   }

   auto value() const noexcept
   {
      return m_elapsed;
   }

private:
   Duration m_elapsed {};
   Clock::time_point m_started {};
};



inline Stopwatch::Duration run(
   std::invocable auto const& work
)
{
   Stopwatch sw;
   sw.start();
   work();
   sw.stop();

   return sw.value();
}


inline Stopwatch::Duration run(
   unsigned threads,
   std::invocable auto const& work
)
{
   if (threads < 1)
      return run(work);

   std::vector<Stopwatch> sw;
   sw.resize(threads);

   std::vector<std::jthread> workers;
   workers.reserve(threads);

   std::mutex mtx;
   std::condition_variable cv;
   bool start = false;

   for (unsigned id = 0; id < threads; ++id)
   {
      workers.emplace_back(
         [id, &sw, &mtx, &cv, &start, &work]()
         {
            // start all the workers synchronously
            {
               std::unique_lock l(mtx);
               cv.wait(l, [&start]() { return start; });
            }

            sw[id].start();
            work();
            sw[id].stop();
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

   // calculate the average duration
   Stopwatch::Duration dura = {};
   for (auto& s: sw)
      dura += s.value();

   return dura / threads;
}


class Runner
{
public:
   Runner() = default;

   void add(
      auto&& name,
      unsigned threads,
      std::invocable auto&&  work
   )
   {
      m_bm.emplace_back(
         std::forward<decltype(name)>(name),
         threads,
         std::forward<decltype(work)>(work)
      );
   }

   void run(std::ostream& ss)
   {
      if (m_bm.empty())
         return;

      for (auto& bm: m_bm)
      {
         ss << "Running [" << bm.name << "]..." << std::endl;

         bm.duration = ::Benchmark::run(
            bm.threads,
            bm.work
         );
      }

      ss << "‐------------------------------------" << std::endl;

      auto best = m_bm.front().nanoseconds();

      for (auto& bm : m_bm)
      {
         auto du = bm.nanoseconds();
         if (du < best)
            best = du;
      }

      for (auto& bm: m_bm)
      {
         auto du = bm.nanoseconds();
         auto percent = du * 100.0 / double(best);

         ss << (du / 1000) <<  " µs ("
            << std::setprecision(2) << std::fixed
            << percent << "%) "
            << "in [" << bm.name << "]" << std::endl;

      }

      ss << "‐------------------‐-----------------" << std::endl;
   }

private:
   struct Benchmark
   {
      std::string name;
      unsigned threads;
      std::function<void()> work;
      Stopwatch::Duration duration = {};

      Benchmark(
         std::string_view name, 
         unsigned threads,
         std::function<void()>&& work
      )
         : name(name)
         , threads(threads)
         , work(std::move(work))
      {}

      std::uint64_t nanoseconds() const noexcept
      {
         return std::chrono::duration_cast<
            std::chrono::nanoseconds
         >(duration).count();
      }
   };

   std::vector<Benchmark> m_bm;
};


} // namespace
