#pragma once

#include "benchmark.hpp"
#include "terminal.hpp"


#include <vector>


namespace Benchmark
{


class Runner
{
public:
   Runner(auto&& name, std::uint64_t iterations)
      : m_name{std::forward<decltype(name)>(name)}
      , m_iterations(iterations)
   {
   }

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

   virtual void run();

   std::ostream& out() noexcept
   {
      return m_console.out();
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

   Terminal m_console;
   std::string m_name;
   std::uint64_t m_iterations;
   std::vector<Benchmark> m_bm;
};


} // namespace
