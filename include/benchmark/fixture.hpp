#pragma once


#include <concepts>
#include <functional>


namespace Benchmark
{


using Counter = std::uint64_t;
using Tid = unsigned;


template <typename T>
concept SimpleBenchmark =
   std::is_invocable_r_v<Counter, T, Counter, Tid>;


struct Fixture
{
   using Ptr = std::unique_ptr<Fixture>;

   virtual ~Fixture() = default;

   virtual void initialize(unsigned threads) {}
   virtual void prologue(Tid tid) {}
   virtual Counter run(Counter iterations, Tid tid) = 0;
   virtual void epilogue(Tid tid) {}
   virtual void finalize() {}

   template <class T, class... Args>
   static Ptr make(Args... args)
   {
      return std::make_unique<T>(
         std::forward<Args>(args)...
      );
   }
};


struct SimpleFixture final
   : public Fixture
{
public:
   SimpleFixture(SimpleBenchmark auto&& work)
      : m_work(std::forward<decltype(work)>(work))
   {}

   static Fixture::Ptr make(SimpleBenchmark auto&& work)
   {
      return std::make_unique<SimpleFixture>(
         std::forward<decltype(work)>(work)
      );
   }

   Counter run(Counter iterations, Tid tid) override
   {
      return m_work(iterations, tid);
   }

private:
   std::function<Counter(Counter, Tid)> m_work;
};



} // namespace
