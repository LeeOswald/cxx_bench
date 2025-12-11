#include <atomic>
#include <iostream>
#include <thread>

#include "benchmark.hpp"


namespace
{

namespace __
{

using Integer = std::size_t;
using AtomicInteger = std::atomic<Integer>;

BM_DONT_OPTIMIZE void basicIncrement(
   Integer& v,
   std::size_t iterations
) noexcept
{
   if (iterations < 1)
      return;

   while (iterations-- > 0)
      v++;
}

BM_DONT_OPTIMIZE void volatileIncrement(
   Integer volatile& v, 
   std::size_t iterations
) noexcept
{
   if (iterations < 1)
      return;

   while (iterations-- > 0)
      v = v + 1;
}

BM_DONT_OPTIMIZE void atomicIncrement(
   AtomicInteger& v,
   std::memory_order order,
   std::size_t iterations
) noexcept
{
   if (iterations < 1)
      return;

   while (iterations-- > 0)
      v.fetch_add(1, order);
}


} // namespace


inline void bench() noexcept
{
   constexpr std::size_t iterations = 100000000ULL;

   Benchmark::Runner r("Atomic operations speed");

   r.add(
      "ST integer increment",
      iterations,
      1,
      [iterations]()
      {
         static __::Integer x = 0;
         __::basicIncrement(x, iterations);
      }
   );

   r.add(
      "ST volatile integer increment",
      iterations,
      1,
      [iterations]()
      {
         static volatile __::Integer x = 0;
         __::volatileIncrement(x, iterations);
      }
   );

   r.add(
      "MT volatile integer increment",
      iterations,
      2,
      [iterations]()
      {
         // shared between threads
         static volatile __::Integer x = 0;
         __::volatileIncrement(x, iterations);
      }
   );

   r.add(
       "ST atomic increment relaxed",
       iterations,
       1,
       [iterations]()
       {
          static  __::AtomicInteger x = 0;
          __::atomicIncrement(
             x,
             std::memory_order_relaxed,
             iterations
         );
       }
   );

   r.add(
       "ST atomic increment acq_rel",
       iterations,
       1,
       [iterations]()
       {
          static  __::AtomicInteger x = 0;
          __::atomicIncrement(
             x,
             std::memory_order_acq_rel,
             iterations
         );
       }
   );

   r.add(
       "MT atomic increment relaxed",
       iterations,
       2,
       [iterations]()
       {
          // shared between threads
          static  __::AtomicInteger x = 0;
          __::atomicIncrement(
             x,
             std::memory_order_relaxed,
             iterations
          );
       }
   );

   r.add(
       "MT atomic increment acq_rel",
       iterations,
       2,
       [iterations]()
       {
          // shared between threads
          static  __::AtomicInteger x = 0;
          __::atomicIncrement(
             x,
             std::memory_order_acq_rel,
             iterations
          );
       }
   );

   r.run(std::cout);
}


} // namespace


int main()
{
   bench();
   return 0;
}
