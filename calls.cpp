#include <functional>
#include <iostream>
#include <memory>

#include "benchmark.hpp"


namespace
{

std::size_t g_result = 0;

__attribute__((noinline)) void freeFun(std::size_t v)
{
      g_result += v;
}


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
   __attribute__((noinline)) void virtualCall(std::size_t v) override
   {
      m_result += v;
   }
};


__attribute__((noinline)) void benchmark(A* a)
{
   constexpr std::size_t iterations = 100000000ULL;

   Benchmark::Runner r("Function call speed");

   r.add(
      "free function",
      iterations,
      1,
      [iterations]()
      {
         auto c = iterations;
         while (c--)
            freeFun(1);
      }
   );

   r.add(
      "normal method",
      iterations,
      1,
      [iterations, a]()
      {
         auto c = iterations;
         while (c--)
            a->normalCall(1);
      }
   );

   r.add(
      "virtual method",
      iterations,
      1,
      [iterations, a]()
      {
         auto c = iterations;
         while (c--)
            a->virtualCall(1);
      }
   );

   std::function<void(std::size_t)> fun = freeFun;

   r.add(
      "std::function -> free function",
      iterations,
      1,
      [iterations, &fun]()
      {
         auto c = iterations;
         while (c--)
            fun(1);
      }
   );

   std::function<void(std::size_t)> fun0 = 
      std::bind(
         &A::normalCall, 
         a, 
         std::placeholders::_1
      );

   r.add(
      "std::function+bind -> method",
      iterations,
      1,
      [iterations, &fun0]()
      {
         auto c = iterations;
         while (c--)
            fun0(1);
      }
   );

   std::function<void(std::size_t)> fun1 =
      std::bind(
         &A::virtualCall,
         a,
         std::placeholders::_1
      );

   r.add(
      "std::function+bind -> virtual method",
      iterations,
      1,
      [iterations, &fun1]()
      {
         auto c = iterations;
         while (c--)
            fun1(1);
      }
   );

   std::function<void(std::size_t)> lam0 = 
   [a](std::size_t v)
   {
      a->normalCall(v);
   };

   r.add(
      "std::function+lambda -> method",
      iterations,
      1,
      [iterations, &lam0]()
      {
         auto c = iterations;
         while (c--)
            lam0(1);
      }
   );

   std::function<void(std::size_t)> lam1 = 
   [a](std::size_t v)
   {
      a->virtualCall(v);
   };

   r.add(
      "std::function+lambda -> virtual method",
      iterations,
      1,
      [iterations, &lam1]()
      {
         auto c = iterations;
         while (c--)
            lam1(1);
      }
   );

   r.run(std::cout, false);
}

} // namespace


int main()
{
   auto a = std::make_unique<B>();

   benchmark(a.get());

   return 0;
}
