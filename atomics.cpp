#include <atomic>
#include <iostream>
#include <thread>

#include "benchmark.hpp"


namespace
{

namespace __
{

using CounterType = std::size_t;
using Counter = std::atomic<CounterType>;


inline void  nonAtomicIncrement(
   CounterType volatile& v, 
   std::size_t iterations
)
{
   if (iterations < 1)
      return;

   while (iterations-- > 0)
      v = v + 1;
}

inline void  atomicIncrement(
   Counter& v,
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

   Benchmark::Runner r;

   r.add(
      "non-atomic increment",
      iterations,
      1,
      [iterations]()
      {
         static volatile __::CounterType x = 0;
         __::nonAtomicIncrement(x, iterations);
      }
   );

   r.add(
       "ST atomic increment relaxed",
       iterations,
       1,
       [iterations]()
       {
          static  __::Counter x = 0;
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
          static  __::Counter x = 0;
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
       2,//std::thread::hardware_concurrency(),
       [iterations]()
       {
          static  __::Counter x = 0;
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
       2,//std::thread::hardware_concurrency(),
       [iterations]()
       {
          static  __::Counter x = 0;
          __::atomicIncrement(
             x,
             std::memory_order_acq_rel,
             iterations
          );
       }
   );

   r.run(std::cout, true);
}


} // namespace


int main()
{
   bench();
   return 0;
}
