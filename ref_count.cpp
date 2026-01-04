#include <atomic>
#include <iostream>
#include <mutex>

#include "runner.hpp"
#include "random.hpp"

#include <pthread.h>


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

void bench(
   std::uint64_t iterations,
   Object& o
   ) noexcept
{
   Refc t = 0;
   while (iterations--)
   {
      t += o->add_ref();
   }

   g_dummy = t;
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

template <class Base>
struct PosixMutex : public IRefCounted, public Base
{
   ~PosixMutex()
   {
      ::pthread_mutex_destroy(&m_mu);
   }

   PosixMutex() noexcept
   {
      ::pthread_mutex_init(&m_mu, nullptr);
   }

   Refc add_ref() noexcept override
   {
      ::pthread_mutex_lock(&m_mu);
      auto r = ++m_refs;
      ::pthread_mutex_unlock(&m_mu);
      return r;
   }

   pthread_mutex_t m_mu;
   Refc m_refs = 0;
};

Object posixMutex()
{
   return make<PosixMutex<A>, PosixMutex<B>>();
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
      [](std::uint64_t iterations)
      {
         auto v = nonAtomic();
         bench(iterations, v);
      }
   );

   r.add(
      "volatile non-atomic counter",
      1,
      [](std::uint64_t iterations)
      {
         auto v = nonAtomicVolatile();
         bench(iterations, v);
      }
   );

   auto relaxed = [](std::uint64_t iterations)
   {
      auto v = atomic<std::memory_order_relaxed>();
      bench(iterations, v);
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

   auto acq_rel = [](std::uint64_t iterations)
   {
      auto v = atomic<std::memory_order_acq_rel>();
      bench(iterations, v);
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

   auto seq_cst = [](std::uint64_t iterations)
   {
      auto v = atomic<std::memory_order_seq_cst>();
      bench(iterations, v);
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

   auto mutexed = [](std::uint64_t iterations)
   {
      auto v = mutex();
      bench(iterations, v);
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

   auto pmutexed = [](std::uint64_t iterations)
   {
      auto v = posixMutex();
      bench(iterations, v);
   };

   r.add(
      "counter + posix mutex",
      1,
      pmutexed
   );

   r.add(
      2,
      pmutexed
   );

   r.add(
      4,
      pmutexed
   );

   r.add(
      8,
      pmutexed
   );

   r.run();

   return 0;
}

