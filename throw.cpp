#include <algorithm>
#include <exception>
#include <iostream>
#include <vector>

#include "benchmark/runner.hpp"


namespace
{

struct Bang
{
};


std::size_t thrower(int bang, std::size_t v)
{
   if (bang > 0)
      throw Bang{};

   return v + bang;
}

std::size_t nothrowFor(
   const std::vector<int>& bangs,
   std::uint64_t iterations
)
{
   std::size_t dummy = 0;
   std::size_t current = 0;
   std::size_t count = bangs.size();
   while (iterations--)
   {
      dummy = thrower(bangs[current], dummy);

      if (++current == count)
         current = 0;
   }

   return dummy;
}

std::size_t throwFor(
   const std::vector<int>& bangs,
   std::uint64_t iterations
)
{
   std::size_t dummy = 0;
   std::size_t current = 0;
   std::size_t count = bangs.size();
   while (iterations--)
   {
      try
      {

         dummy = thrower(bangs[current], dummy);
      }
      catch (Bang&)
      {
      }
      catch (...)
      {
         std::terminate();
      }

      if (++current == count)
         current = 0;
   }

   return dummy;
}



} // namespace


int main()
{
   constexpr std::uint64_t iterations = 1000000ULL;
   Benchmark::Runner r("Exception performance", iterations);

   volatile std::size_t dummy = 0;
   std::vector<int> nobangs;
   std::fill_n(
      std::back_inserter(nobangs),
      iterations,
      0
   );

   r.add(
      "baseline",
      1,
      [&nobangs, &dummy](Benchmark::Counter iterations, Benchmark::Tid)
         -> Benchmark::Counter
      {
         dummy = nothrowFor(nobangs, iterations);
         return 0;
      }
   );

   r.add(
      "try+catch (xcpt not fired)",
      1,
      [&nobangs, &dummy](Benchmark::Counter iterations, Benchmark::Tid)
         -> Benchmark::Counter
      {
         dummy = throwFor(nobangs, iterations);
         return 0;
      }
   );

   std::vector<int> bangs;
   std::fill_n(
      std::back_inserter(bangs),
      iterations,
      1
   );

   auto ttc = [&bangs, &dummy](Benchmark::Counter iterations, Benchmark::Tid)
         -> Benchmark::Counter
   {
      dummy = throwFor(bangs, iterations);
      return 0;
   };

   r.add(
      "try+throw+catch",
      1,
      ttc
   );

   r.add(
      2,
      ttc
   );

   r.add(
      4,
      ttc
   );

   r.run();

   return 0;
}
