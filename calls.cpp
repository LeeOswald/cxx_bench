#include <functional>
#include <iostream>
#include <memory>

#include "benchmark.hpp"


namespace
{

using Type = std::size_t;


BM_DONT_OPTIMIZE Type freeFun(Type a)
{
   return a;
}


struct A
{
   BM_DONT_OPTIMIZE Type normalCall(Type a)
   {
      return a;
   }

   virtual Type virtualCall(Type a) = 0;
};


struct B : public A
{
   BM_DONT_OPTIMIZE Type virtualCall(Type a) override
   {
      return a;
   }
};


void benchmark(A* a)
{
   constexpr std::size_t iterations = 1000000000ULL;

   Benchmark::Runner r("Function call speed");

   r.add(
      "free function",
      iterations,
      1,
      [iterations]()
      {
         auto c = iterations;
         while (c--)
            freeFun(c);
      }
   );

   r.add(
      "class  method",
      iterations,
      1,
      [iterations, a]()
      {
         auto c = iterations;
         while (c--)
            a->normalCall(c);
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
            a->virtualCall(c);
      }
   );

   std::function<Type(Type)> fun = freeFun;

   r.add(
      "std::function -> free function",
      iterations,
      1,
      [iterations, &fun]()
      {
         auto c = iterations;
         while (c--)
            fun(c);
      }
   );

   std::function<Type(Type)> fun0 =
      std::bind(
         &A::normalCall,
         a,
         std::placeholders::_1
      );

   r.add(
      "std::function + std::bind -> method",
      iterations,
      1,
      [iterations, &fun0]()
      {
         auto c = iterations;
         while (c--)
            fun0(c);
      }
   );

   std::function<Type(Type)> fun1 =
      std::bind(
         &A::virtualCall,
         a,
         std::placeholders::_1
      );

   r.add(
      "std::function + std::bind -> virtual method",
      iterations,
      1,
      [iterations, &fun1]()
      {
         auto c = iterations;
         while (c--)
            fun1(c);
      }
   );

   std::function<Type(Type)> lam0 =
   [a](Type v)
   {
      return a->normalCall(v);
   };

   r.add(
      "std::function + lambda -> method",
      iterations,
      1,
      [iterations, &lam0]()
      {
         auto c = iterations;
         while (c--)
            lam0(c);
      }
   );

   std::function<Type(Type)> lam1 =
   [a](Type v)
   {
      return a->virtualCall(v);
   };

   r.add(
      "std::function + lambda -> virtual method",
      iterations,
      1,
      [iterations, &lam1]()
      {
         auto c = iterations;
         while (c--)
            lam1(c);
      }
   );

   r.run(std::cout);
}

} // namespace


int main()
{
   auto a = std::make_unique<B>();

   benchmark(a.get());

   return 0;
}
