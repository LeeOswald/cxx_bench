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
      bool first = true;
      for (auto n : threads)
      {
         m_items.emplace_back(
            name,
            n,
            SimpleFixture::make(std::forward<decltype(work)>(work)),
            first
         );

         if (first)
            first = false;
      }
   }

   virtual void run();

   std::ostream& out() noexcept
   {
      return m_console.out();
   }

private:
   struct Item
   {
      std::string name;
      unsigned threads;
      Fixture::Ptr work;
      bool group;
      Data data;

      Item(
         std::string_view name,
         unsigned threads,
         Fixture::Ptr&& work,
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
   Counter m_iterations;
   std::vector<Item> m_items;
};


} // namespace
