#pragma once

#include "benchmark.hpp"
#include "terminal.hpp"

#include <initializer_list>
#include <vector>


namespace Benchmark
{


class Runner
{
public:
   Runner(std::string_view name, Counter iterations)
      : m_name(name)
      , m_iterations(iterations)
   {
   }

   void add(
      std::string_view name,
      SimpleBenchmark auto&& work,
      std::initializer_list<unsigned> threads = {1}
   )
   {
      m_bm.emplace_back(
         name,
         threads,
         SimpleFixture::make(std::forward<decltype(work)>(work))
      );
   }

   void add(
      std::string_view name,
      Fixture::Ptr&& work,
      std::initializer_list<unsigned> threads = {1}
   )
   {
      m_bm.emplace_back(
         name,
         threads,
         std::move(work)
      );
   }

   virtual void run();

   auto& out() noexcept
   {
      return m_console.out();
   }

   auto& err() noexcept
   {
      return m_console.err();
   }

private:
   struct Bm
   {
      std::string name;
      std::vector<unsigned> threads;
      Fixture::Ptr work;
      std::vector<Data> data;

      Bm(
         std::string_view name,
         std::initializer_list<unsigned> threads,
         Fixture::Ptr&& work
      )
         : name(name)
         , threads(threads)
         , work(std::move(work))
      {}
   };

   virtual void printCaption();
   virtual void printFooter();

   virtual void printRunning(
      std::size_t index,
      std::size_t variant
   );

   virtual void printResult(
      std::size_t index,
      std::size_t variant
   );

   virtual void printHeader();

   Terminal m_console;
   std::string m_name;
   Counter m_iterations;
   std::vector<Bm> m_bm;
};


} // namespace
