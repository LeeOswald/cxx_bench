#include <atomic>
#include <iostream>
#include <mutex>
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
      ++v;
}

void volatileIncrement(
   Integer volatile& v, 
   std::size_t iterations
) noexcept
{
   if (iterations < 1)
      return;

   while (iterations-- > 0)
      v = v + 1;
}

void atomicIncrement(
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


struct IntegerWithMutex
{
   std::mutex m;
   Integer v = 0;

   IntegerWithMutex() = default;

   void increment()
   {
      std::lock_guard l(m);
      ++v;
   }
};

void mutexIncrement(
   IntegerWithMutex& v,
   std::size_t iterations
) noexcept
{
   if (iterations < 1)
      return;

   while (iterations-- > 0)
      v.increment();
}

} // namespace


inline void bench() noexcept
{
   constexpr std::size_t iterations = 100000000ULL;

   Benchmark::Runner r(
      "Atomic operations speed",
      iterations
   );

   r.add(
      "integer increment",
      1,
      [](std::uint64_t iterations)
      {
         static __::Integer x = 0;
         __::basicIncrement(x, iterations);
      }
   );

   r.add(
      "volatile integer increment (1 thrd)",
      1,
      [](std::uint64_t iterations)
      {
         static volatile __::Integer x = 0;
         __::volatileIncrement(x, iterations);
      }
   );

   r.add(
      "volatile integer increment (2 thrd)",
      2,
      [](std::uint64_t iterations)
      {
         // shared between threads
         static volatile __::Integer x = 0;
         __::volatileIncrement(x, iterations);
      }
   );

   r.add(
      "volatile integer increment (4 thrd)",
      4,
      [](std::uint64_t iterations)
      {
         // shared between threads
         static volatile __::Integer x = 0;
         __::volatileIncrement(x, iterations);
      }
   );

   r.add(
       "atomic increment relaxed (1 thrd)",
       1,
       [](std::uint64_t iterations)
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
       "atomic increment relaxed (2 thrd)",
       2,
       [](std::uint64_t iterations)
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
       "atomic increment relaxed (4 thrd)",
       4,
       [](std::uint64_t iterations)
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
       "atomic increment acq_rel (1 thrd)",
       1,
       [](std::uint64_t iterations)
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
       "atomic increment acq_rel (2 thrd)",
       2,
       [](std::uint64_t iterations)
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
       "atomic increment acq_rel (4 thrd)",
       4,
       [](std::uint64_t iterations)
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
       "std::mutex increment (1 thrd)",
       1,
       [](std::uint64_t iterations)
       {
          static  __::IntegerWithMutex x;
          __::mutexIncrement(
             x,
             iterations
         );
       }
   );

   r.add(
       "std::mutex increment (2 thrd)",
       2,
       [](std::uint64_t iterations)
       {
          static  __::IntegerWithMutex x;
          __::mutexIncrement(
             x,
             iterations
         );
       }
   );

   r.add(
       "std::mutex increment (4 thrd)",
       4,
       [](std::uint64_t iterations)
       {
          static  __::IntegerWithMutex x;
          __::mutexIncrement(
             x,
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
