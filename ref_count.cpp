#include <atomic>
#include <mutex>
#include <vector>

#include "benchmark/runner.hpp"
#include "benchmark/random.hpp"
#include "benchmark/util.hpp"


namespace
{


class Fixture
   : public Benchmark::Fixture
{
public:
   Fixture()
      : m_rand(0)
   {}

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid tid
   ) override
   {
      while (iterations--)
      {
         next(tid)->add_ref();
      }

      return 0;
   }

   void finalize() override
   {
      m_objs.clear();
   }

protected:
   class A {};
   class B {};

   using Refc = std::size_t;

   struct IRefCounted
   {
      using Ptr = std::unique_ptr<IRefCounted>;

      virtual ~IRefCounted() = default;
      virtual void add_ref() noexcept = 0;
   };

   static volatile Refc g_dontOptimize;

   Benchmark::Random m_rand;

   std::vector<
      Benchmark::AnyObjectVector<IRefCounted>
   > m_objs;

   std::vector<std::size_t> m_next;

   IRefCounted* next(uint tid) noexcept
   {
      auto idx = m_next[tid]++;
      if (m_next[tid] == m_objs[tid].size())
         m_next[tid] = 0;

      return m_objs[tid][idx].get();
   }
};

volatile Fixture::Refc Fixture::g_dontOptimize = 0;


class NonAtomic
   : public Fixture
{
private:
   template <class Base>
   struct Obj
      : public IRefCounted
      , public Base
   {
      ~Obj()
      {
         g_dontOptimize = m_refs;
      }

      constexpr Obj() noexcept = default;

      void add_ref() noexcept override
      {
         ++m_refs;
      }

      Refc m_refs = 0;
   };

public:
   NonAtomic() = default;

   void initialize(unsigned threads)
   {
      m_next.resize(threads);
      m_objs.resize(threads);

      for (unsigned tid = 0; tid < threads; ++tid)
      {
         m_objs[tid].clear();

         Benchmark::AnyObject<IRefCounted, Obj<A>, Obj<B>>::fill(
            m_objs[tid],
            [this]() { return m_rand() % 3 == 0; },
            64
         );

         m_next[tid] = 0;
      }
   }
};


class NonAtomicVolatile
   : public Fixture
{
private:
   template <class Base>
   struct Obj
      : public IRefCounted
      , public Base
   {
      ~Obj()
      {
         g_dontOptimize = m_refs;
      }

      constexpr Obj() noexcept = default;

      void add_ref() noexcept override
      {
         auto v = m_refs + 1;
         m_refs = v;
      }

      Refc volatile m_refs = 0;
   };

public:
   NonAtomicVolatile() = default;

   void initialize(unsigned threads)
   {
      m_next.resize(threads);
      m_objs.resize(threads);

      for (unsigned tid = 0; tid < threads; ++tid)
      {
         m_objs[tid].clear();

         Benchmark::AnyObject<IRefCounted, Obj<A>, Obj<B>>::fill(
            m_objs[tid],
            [this]() { return m_rand() % 3 == 0; },
            64
         );

         m_next[tid] = 0;
      }
   }
};


template <std::memory_order Order>
class Atomic
   : public Fixture
{
private:
   template <class Base>
   struct Obj
      : public IRefCounted
      , public Base
   {
      ~Obj()
      {
         g_dontOptimize = m_refs.load();
      }

      constexpr Obj() noexcept = default;

      void add_ref() noexcept override
      {
         m_refs.fetch_add(1, Order);
      }

      std::atomic<Refc> m_refs = 0;
   };

public:
   Atomic() = default;

   void initialize(unsigned threads)
   {
      m_next.resize(threads);
      m_objs.resize(threads);

      for (unsigned tid = 0; tid < threads; ++tid)
      {
         m_objs[tid].clear();

         Benchmark::AnyObject<IRefCounted, Obj<A>, Obj<B>>::fill(
            m_objs[tid],
            [this]() { return m_rand() % 3 == 0; },
            64
         );

         m_next[tid] = 0;
      }
   }
};


class Mutex
   : public Fixture
{
private:
   template <class Base>
   struct Obj
      : public IRefCounted
      , public Base
   {
      ~Obj()
      {
         g_dontOptimize = m_refs;
      }

      constexpr Obj() noexcept = default;

      void add_ref() noexcept override
      {
         std::lock_guard l(m_mu);
         ++m_refs;
      }

      std::mutex m_mu;
      Refc m_refs = 0;
   };

public:
   Mutex() = default;

   void initialize(unsigned threads)
   {
      m_next.resize(threads);
      m_objs.resize(threads);

      for (unsigned tid = 0; tid < threads; ++tid)
      {
         m_objs[tid].clear();

         Benchmark::AnyObject<IRefCounted, Obj<A>, Obj<B>>::fill(
            m_objs[tid],
            [this]() { return m_rand() % 3 == 0; },
            64
         );

         m_next[tid] = 0;
      }
   }
};

} // namespace


int main(int argc, char** argv)
{
   auto iterations = Benchmark::getIntArgOr(
      "-n",
      10000000ULL,
      argc,
      argv
   );

   if (iterations < 1)
   {
      std::cerr << "-n must be positive\n";
      return -1;
   }

   Benchmark::Runner r(
      "Reference count performance",
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

