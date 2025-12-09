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

   return dura / sw.size();
}


class Runner
{
public:
   Runner() = default;

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
      if (m_bm.empty())
         return;

      for (auto& bm: m_bm)
      {
         ss << "[" << bm.name << "] ×" << bm.iterations;

         if (bm.threads > 1)
            ss << " ×" << bm.threads << " threads";

         ss << std::endl;

         bm.duration = ::Benchmark::run(
            bm.threads,
            bm.work
         );
      }

      auto line = [&ss]() 
      {
         ss << "-------------------‐--------------"
            << "----------------------" << std::endl;
      };

      line();
      ss << " Total, µs|"
         << " Op, ns|"
         << "   %   |"
         << "    What     " << std::endl;
      line();

      auto best = m_bm.front().nanoseconds();

      for (auto& bm: m_bm)
      {
         auto du = bm.nanoseconds();
         auto op = du / bm.iterations;
         auto percent = du * 100.0 / double(best);

         ss << std::setw(10) << (du / 1000) <<  "|"
            << std::setw(7) << op << "|"
            << std::setw(7) << std::setprecision(2) << std::fixed
            << percent << "| "
            << bm.name << std::endl;

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
      Stopwatch::Duration duration = {};

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
