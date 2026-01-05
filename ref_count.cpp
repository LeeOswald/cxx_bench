#include <atomic>
#include <mutex>

#include "benchmark/runner.hpp"
#include "benchmark/random.hpp"


namespace
{


class Fixture
   : public Benchmark::Fixture
{
public:
   Fixture()
      : m_rand(0)
   {}

   void initialize(unsigned threads) override
   {
      auto o = makeObj();
      m_obj.swap(o);
   }

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid tid
   ) override
   {
      return bench(iterations, m_obj.get());
   }

   void finalize() override
   {
      m_obj.reset();
   }

protected:
   using Refc = std::size_t;

   struct IRefCounted
   {
      using Ptr = std::unique_ptr<IRefCounted>;

      virtual ~IRefCounted() = default;
      virtual Refc add_ref() noexcept = 0;
   };

   static volatile Refc g_dummy;
   Benchmark::Random m_rand;
   IRefCounted::Ptr m_obj;

   Benchmark::Counter bench(
      Benchmark::Counter iterations,
      IRefCounted* o
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

   template <class Type1, class Type2>
   IRefCounted::Ptr makeAnyObj()
   {
      auto id = m_rand();
      if (id % 2 == 0)
         return std::make_unique<Type1>();
      else
         return std::make_unique<Type2>();
   }

   virtual IRefCounted::Ptr makeObj() = 0;
};

volatile Fixture::Refc Fixture::g_dummy = 0;


class NonAtomic
   : public Fixture
{
private:
   class BaseA {};
   class BaseB {};

   template <class Base>
   struct Obj
      : public IRefCounted
      , public Base
   {
      constexpr Obj() noexcept = default;

      Refc add_ref() noexcept override
      {
         return ++m_refs;
      }

      Refc m_refs = 0;
   };

public:
   NonAtomic() = default;

   IRefCounted::Ptr makeObj() override
   {
      return makeAnyObj<Obj<BaseA>, Obj<BaseB>>();
   }
};


class NonAtomicVolatile
   : public Fixture
{
private:
   class BaseA {};
   class BaseB {};

   template <class Base>
   struct Obj
      : public IRefCounted
      , public Base
   {
      constexpr Obj() noexcept = default;

      Refc add_ref() noexcept override
      {
         auto v = m_refs + 1;
         m_refs = v;
         return v;
      }

      Refc volatile m_refs = 0;
   };

public:
   NonAtomicVolatile() = default;

   IRefCounted::Ptr makeObj() override
   {
      return makeAnyObj<Obj<BaseA>, Obj<BaseB>>();
   }
};


template <std::memory_order Order>
class Atomic
   : public Fixture
{
private:
   class BaseA {};
   class BaseB {};

   template <class Base>
   struct Obj
      : public IRefCounted
      , public Base
   {
      constexpr Obj() noexcept = default;

      Refc add_ref() noexcept override
      {
         return m_refs.fetch_add(1, Order) + 1;
      }

      std::atomic<Refc> m_refs = 0;
   };

public:
   Atomic() = default;

   IRefCounted::Ptr makeObj() override
   {
      return makeAnyObj<Obj<BaseA>, Obj<BaseB>>();
   }
};


class Mutex
   : public Fixture
{
private:
   class BaseA {};
   class BaseB {};

   template <class Base>
   struct Obj
      : public IRefCounted
      , public Base
   {
      constexpr Obj() noexcept = default;

      Refc add_ref() noexcept override
      {
         std::lock_guard l(m_mu);
         return ++m_refs;
      }

      std::mutex m_mu;
      Refc m_refs = 0;
   };

public:
   Mutex() = default;

   IRefCounted::Ptr makeObj() override
   {
      return makeAnyObj<Obj<BaseA>, Obj<BaseB>>();
   }
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

