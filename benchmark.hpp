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
#include <unistd.h>


namespace Benchmark
{


class CpuMeter final
{
public:
   using Duration = std::chrono::milliseconds;

   struct CpuTimes
   {
      Duration user;
      Duration system;
   };

   CpuMeter() noexcept = default;

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

   CpuTimes value() const noexcept
   {
      return CpuTimes {
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

   std::chrono::nanoseconds stop() noexcept
   {
      auto delta = Clock::now() - m_started;
      m_elapsed += delta;

      return std::chrono::duration_cast<
         std::chrono::nanoseconds
      >(delta);
   }

   std::chrono::nanoseconds value() const noexcept
   {
      return std::chrono::duration_cast<
         std::chrono::nanoseconds
      >(m_elapsed);
   }

private:
   Duration m_elapsed = {};
   Clock::time_point m_started = {};
};


struct Timings final
{
   std::chrono::nanoseconds duration;
   std::optional<CpuMeter::CpuTimes> cpu;
};


inline Timings run(
   std::invocable auto const& work
)
{
   CpuMeter cm;
   Stopwatch sw;

   cm.start();
   sw.start();

   work();

   sw.stop();
   cm.stop();

   return Timings {
      sw.value(),
      cm.value()
   };
}


inline Timings run(
   unsigned threads,
   std::invocable auto const& work
)
{
   if (threads < 2)
      return run(work);

   std::vector<CpuMeter> cpu;
   cpu.resize(threads);
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
         [id, &cpu, &sw, &mtx, &cv, &start, &work]()
         {
            // start all the workers synchronously
            {
               std::unique_lock l(mtx);
               cv.wait(l, [&start]() { return start; });
            }

            cpu[id].start();
            sw[id].start();

            work();

            sw[id].stop();
            cpu[id].stop();
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

   // calculate the average duration & CPU usage
   std::chrono::nanoseconds dura = {};
   for (auto& s: sw)
   {
      dura += s.value();
   }
   dura /= sw.size();

   CpuMeter::CpuTimes cpuTimes = {{}, {}};
   for (auto& c : cpu)
   {
      auto t = c.value();
      cpuTimes.user += t.user;
      cpuTimes.system += t.system;
   }
   cpuTimes.user /= cpu.size();
   cpuTimes.system /= cpu.size();

   return Timings {
      dura,
      cpuTimes
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

   virtual void run(std::ostream& ss, bool showCpuTimes)
   {
      auto thickLine = [&ss]()
      {
         ss << "≈================================="
            << "======================" << std::endl;
      };

      thickLine();
      ss << m_name << std::endl;

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

      auto line = [&ss]() 
      {
         ss << "-------------------‐--------------"
            << "----------------------" << std::endl;
      };

      line();
      ss << "  # "
         << " Total, µs |"
         << " Op, ns |"
         << "    %    |";
      if (showCpuTimes)
         ss << " CPU (u/s), ms";

      ss << std::endl;
      line();

      auto best = m_bm.front().timings.duration.count();

      id = 0;
      for (auto& bm: m_bm)
      {
         auto du = bm.timings.duration.count();
         auto op = du / bm.iterations;
         auto percent = du * 100.0 / double(best);

         ss << std::setw(3) << id++ << " |"
            << std::setw(10) << (du / 1000) <<  "|"
            << std::setw(7) << op << " | "
            << std::setw(7) << std::setprecision(2) << std::fixed
            << percent;

         if (showCpuTimes)
         {
            if (bm.timings.cpu)
            {
               ss << " | "
                  << bm.timings.cpu->user.count()
                  << "/" 
                  << bm.timings.cpu->system.count();
            }
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
