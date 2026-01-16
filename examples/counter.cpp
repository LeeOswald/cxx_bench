#include <atomic>
#include <cmath>
#include <mutex>

#include "benchmark/random.hpp"
#include "benchmark/runner.hpp"
#include "benchmark/util.hpp"


namespace
{


class Fixture
   : public Benchmark::Fixture
{
public:
   Fixture()
      : m_rand(Benchmark::tid())
   {}

protected:
   using Counter = std::int64_t;

   Counter heavyFun() noexcept
   {
      return static_cast<Counter>(
         std::floor(
            std::sin(m_rand()) *
            std::cos(m_rand())
         )
      ) & 1;
   }

   static volatile Counter g_dontOptimize;

   Benchmark::Random m_rand;
};

volatile Fixture::Counter Fixture::g_dontOptimize = 0;


class NonAtomic
   : public Fixture
{
public:
   NonAtomic() = default;

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid tid
   ) override
   {
      while (iterations--)
      {
         m_counter += heavyFun();
      }

      g_dontOptimize = m_counter;
      return 0;
   }

private:
   Counter m_counter = 0;
};


class NonAtomicVolatile
   : public Fixture
{
public:
   NonAtomicVolatile() = default;

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid tid
   ) override
   {
      while (iterations--)
      {
         m_counter = m_counter + heavyFun();
      }

      g_dontOptimize = m_counter;
      return 0;
   }

private:
   Counter volatile m_counter = 0;
};


template <std::memory_order Order>
class Atomic
   : public Fixture
{
public:
   Atomic() = default;

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid tid
   ) override
   {
      while (iterations--)
      {
         m_counter.fetch_add(
            heavyFun(),
            Order
         );
      }

      g_dontOptimize = m_counter;
      return 0;
   }

private:
   std::atomic<Counter> m_counter = 0;
};


class Mutex
   : public Fixture
{
public:
   Mutex() = default;

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid tid
   ) override
   {
      while (iterations--)
      {
         auto t = heavyFun();

         std::lock_guard l(m_mu);
         m_counter += t;
      }

      g_dontOptimize = m_counter;
      return 0;
   }

private:
   std::mutex m_mu;
   Counter m_counter = 0;
};

} // namespace


int main(int argc, char** argv)
{
   Benchmark::CmdLine cmd(argc, argv);
   std::uint64_t iterations = 10000000ULL;

   Benchmark::bindArg(
      cmd,
      "-n",
      iterations,
      "-n must be a positive integer"
   );

   Benchmark::Runner r(
      "Counter performance",
      iterations
   );


   r.add(
      "non-atomic counter",
      Benchmark::Fixture::make<NonAtomic>()
   );

   r.add(
      "non-atomic volatile counter",
      Benchmark::Fixture::make<NonAtomicVolatile>()
   );

   r.add(
      "atomic counter (relaxed)",
      Benchmark::Fixture::make<Atomic<std::memory_order_relaxed>>(),
      { 1, 2, 4, 8 }
   );

   r.add(
      "atomic counter (acq_rel)",
      Benchmark::Fixture::make<Atomic<std::memory_order_acq_rel>>(),
      { 1, 2, 4, 8 }
   );

   r.add(
      "atomic counter (seq_cst)",
      Benchmark::Fixture::make<Atomic<std::memory_order_seq_cst>>(),
      { 1, 2, 4, 8 }
   );

   r.add(
      "mutex + counter",
      Benchmark::Fixture::make<Mutex>(),
      { 1, 2, 4, 8 }
   );

   r.run();

   return 0;
}

