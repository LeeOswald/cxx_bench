#include <atomic>
#include <iostream>
#include <mutex>

#include "benchmark/runner.hpp"
#include "benchmark/random.hpp"


namespace
{


using Refc = std::size_t;

struct IRefCounted
{
   using Ptr = std::unique_ptr<IRefCounted>;

   virtual ~IRefCounted() = default;
   virtual Refc add_ref() noexcept = 0;
};

struct A {};
struct B {};

using Object = IRefCounted::Ptr;
volatile Refc g_dummy = 0;

Benchmark::Counter bench(
   Benchmark::Counter iterations,
   Object& o
   ) noexcept
{
   Refc t = 0;
   while (iterations--)
   {
      t += o->add_ref();
   }

   g_dummy = t;
   return 0;
}

template <class A, class B>
Object make()
{
   Benchmark::Random r(0);

   auto id = r();
   if (id % 2 == 0)
      return Object(new A());
   else
      return Object(new B());
}

template <class Base>
struct NonAtomic : public IRefCounted, public Base
{
   constexpr NonAtomic() noexcept = default;

   Refc add_ref() noexcept override
   {
      return ++m_refs;
   }

   Refc m_refs = 0;
};

Object nonAtomic()
{
   return make<NonAtomic<A>, NonAtomic<B>>();
}

template <class Base>
struct Volatile : public IRefCounted, public Base
{
   constexpr Volatile() noexcept = default;

   Refc add_ref() noexcept override
   {
      m_refs = m_refs + 1;
      return m_refs;
   }

   Refc volatile m_refs = 0;
};

Object nonAtomicVolatile()
{
   return make<Volatile<A>, Volatile<B>>();
}

template <class Base, std::memory_order Order>
struct Atomic : public IRefCounted, public Base
{
   constexpr Atomic() noexcept = default;

   Refc add_ref() noexcept override
   {
      return m_refs.fetch_add(1, Order) + 1;
   }

   std::atomic<Refc> m_refs = 0;
};

template <std::memory_order Order>
Object atomic()
{
   return make<Atomic<A, Order>, Atomic<B, Order>>();
}

template <class Base>
struct Mutex : public IRefCounted, public Base
{
   constexpr Mutex() noexcept = default;

   Refc add_ref() noexcept override
   {
      std::lock_guard l(m_mu);
      return ++m_refs;
   }

   std::mutex m_mu;
   Refc m_refs = 0;
};

Object mutex()
{
   return make<Mutex<A>, Mutex<B>>();
};

} // namespace


int main()
{
   constexpr std::size_t Iterations = 100000000ULL;

   Benchmark::Runner r(
      "Reference count implementations",
      Iterations
   );

   r.add(
      "non-atomic counter",
      1,
      [](Benchmark::Counter iterations, Benchmark::Tid)
      {
         auto v = nonAtomic();
         return bench(iterations, v);
      }
   );

   r.add(
      "volatile non-atomic counter",
      1,
      [](Benchmark::Counter iterations, Benchmark::Tid)
      {
         auto v = nonAtomicVolatile();
         return bench(iterations, v);
      }
   );

   auto relaxed = [](Benchmark::Counter iterations, Benchmark::Tid)
   {
      auto v = atomic<std::memory_order_relaxed>();
      return bench(iterations, v);
   };

   r.add(
      "atomic counter (relaxed)",
      1,
      relaxed
   );

   r.add(
      2,
      relaxed
   );

   r.add(
      4,
      relaxed
   );

   r.add(
      8,
      relaxed
   );

   auto acq_rel = [](Benchmark::Counter iterations, Benchmark::Tid)
   {
      auto v = atomic<std::memory_order_acq_rel>();
      return bench(iterations, v);
   };

   r.add(
      "atomic counter (acq_rel)",
      1,
      acq_rel
   );

   r.add(
      2,
      acq_rel
   );

   r.add(
      4,
      acq_rel
   );

   r.add(
      8,
      acq_rel
   );

   auto seq_cst = [](Benchmark::Counter iterations, Benchmark::Tid)
   {
      auto v = atomic<std::memory_order_seq_cst>();
      return bench(iterations, v);
   };

   r.add(
      "atomic counter (seq_cst)",
      1,
      seq_cst
   );

   r.add(
      2,
      seq_cst
   );

   r.add(
      4,
      seq_cst
   );

   r.add(
      8,
      seq_cst
   );

   auto mutexed = [](Benchmark::Counter iterations, Benchmark::Tid)
   {
      auto v = mutex();
      return bench(iterations, v);
   };

   r.add(
      "counter + mutex",
      1,
      mutexed
   );

   r.add(
      2,
      mutexed
   );

   r.add(
      4,
      mutexed
   );

   r.add(
      8,
      mutexed
   );

   r.run();

   return 0;
}

