#include <iostream>
#include <memory>

#include "benchmark.hpp"


namespace
{

struct A
{
   __attribute__((noinline)) void normalCall(std::size_t v)
   {
      m_result += v;
   }

   virtual void virtualCall(std::size_t v) = 0;

   std::size_t m_result;
};


struct B : public A
{
   void virtualCall(std::size_t v) override
   {
      m_result += v;
   }
};


__attribute__((noinline)) void benchmark(A* a)
{
   constexpr std::size_t iterations = 100000000ULL;

   Benchmark::Runner r;

   r.add(
      "non-inline call",
      1,
      [iterations, a]()
      {
         auto c = iterations;
         while (c--)
            a->normalCall(1);
      }
   );

   r.add(
      "virtual call",
      1,
      [iterations, a]()
      {
         auto c = iterations;
         while (c--)
            a->virtualCall(1);
      }
   );

   std::cout << "Running " << iterations << " iterations:\n";

   r.run(std::cout);
}

} // namespace


int main()
{
   auto a = std::make_unique<B>();

   benchmark(a.get());

   return 0;
}
