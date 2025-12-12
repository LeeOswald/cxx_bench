#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "benchmark.hpp"


namespace
{

using Type = std::size_t;


struct FreeData
{
   Type v;

   constexpr FreeData(Type v) noexcept
      : v(v)
   {}
};

Type freeFun(FreeData* this_, Type a) noexcept
{
   this_->v += a;
   return this_->v;
}


std::vector<std::unique_ptr<FreeData>>
   makeFreeFun(std::size_t n)
{
   std::vector<std::unique_ptr<FreeData>> result;
   result.reserve(n);
   for (std::size_t i = 0; i < n; ++i)
      result.emplace_back(new FreeData(i));

   return result;
}


struct A
{
   virtual ~A() = default;

   constexpr A(Type v) noexcept
      : m_v(v)
   {}

   Type methodCall(Type a)
   {
      m_v += a;
      return m_v;
   }

   virtual Type virtualCall(Type a) = 0;

   Type (*pseudoVirtualCall)(A*, Type) = nullptr;

   Type m_v = 0;
};


struct B : public A
{
   Type virtualCall(Type a) override
   {
      m_v += a;
      return m_v;
   }

   static Type pseudoVirtualImpl(
      A* this_,
      Type a
   )
   {
      this_->m_v += a;
      return this_->m_v;
   }

   B(Type v) noexcept
      : A(v)
   {
      A::pseudoVirtualCall = &B::pseudoVirtualImpl;
   }
};

struct C : public A
{
   Type virtualCall(Type a) override
   {
      m_v += a;
      return m_v;
   }

   static Type pseudoVirtualImpl(
      A* this_,
      Type a
   )
   {
      this_->m_v += a;
      return this_->m_v;
   }

   C(Type v) noexcept
      : A(v)
   {
      A::pseudoVirtualCall = &C::pseudoVirtualImpl;
   }
};


std::vector<std::unique_ptr<A>>
   makeABC(std::size_t n)
{
   std::vector<std::unique_ptr<A>> result;
   result.reserve(n);

   Benchmark::Random r(0);

   for (std::size_t i = 0; i < n; ++i)
   {
      auto id = r();
      if (id % 2 == 0)
         result.emplace_back(new B(id));
      else
         result.emplace_back(new C(id));
   }

   return result;
}


} // namespace


int main()
{
   constexpr std::size_t iterations = 1000000000ULL;

   Benchmark::Runner r("Function call speed");

   auto ff = makeFreeFun(1024);

   r.add(
      "free function",
      iterations,
      1,
      [iterations, &ff]()
      {
         auto c = iterations;
         std::size_t current = 0;
         auto count = ff.size();
         while (c--)
         {
            freeFun(ff[current].get(), c);
            if (++current == count)
               current = 0;
         }
      }
   );

   auto abc = makeABC(1024);

   r.add(
      "class  method",
      iterations,
      1,
      [iterations, &abc]()
      {
         auto c = iterations;
         std::size_t current = 0;
         auto count = abc.size();
         while (c--)
         {
            abc[current].get()->methodCall(c);
            if (++current == count)
               current = 0;
         }
      }
   );

   r.add(
      "virtual method",
      iterations,
      1,
      [iterations, &abc]()
      {
         auto c = iterations;
         std::size_t current = 0;
         auto count = abc.size();
         while (c--)
         {
            abc[current].get()->virtualCall(c);
            if (++current == count)
               current = 0;
         }
      }
   );

   r.add(
      "pseudo-virtual method",
      iterations,
      1,
      [iterations, &abc]()
      {
         auto c = iterations;
         std::size_t current = 0;
         auto count = abc.size();
         while (c--)
         {
            auto o = abc[current].get();
            o->pseudoVirtualCall(o, c);
            if (++current == count)
               current = 0;
         }
      }
   );

   std::function<Type(FreeData*,Type)> fun = freeFun;

   r.add(
      "std::function -> free function",
      iterations,
      1,
      [iterations, &ff, &fun]()
      {
         auto c = iterations;
         std::size_t current = 0;
         auto count = ff.size();
         while (c--)
         {
            fun(ff[current].get(), c);
            if (++current == count)
               current = 0;
         }
      }
   );

   std::function<Type(A*,Type)> fun0 =
      std::bind(
         &A::methodCall,
         std::placeholders::_1,
         std::placeholders::_2
      );

   r.add(
      "std::function + std::bind -> method",
      iterations,
      1,
      [iterations, &abc, &fun0]()
      {
         auto c = iterations;
         std::size_t current = 0;
         auto count = abc.size();
         while (c--)
         {
            auto o = abc[current].get();
            fun0(o, c);
            if (++current == count)
               current = 0;
         }
      }
   );

   std::function<Type(A*,Type)> fun1 =
      std::bind(
         &A::virtualCall,
         std::placeholders::_1,
         std::placeholders::_2
      );

   r.add(
      "std::function + std::bind -> virtual method",
      iterations,
      1,
      [iterations, &abc, &fun1]()
      {
         auto c = iterations;
         std::size_t current = 0;
         auto count = abc.size();
         while (c--)
         {
            auto o = abc[current].get();
            fun1(o, c);
            if (++current == count)
               current = 0;
         }
      }
   );

   std::function<Type(A*,Type)> lam0 =
   [](A* a, Type v)
   {
      return a->methodCall(v);
   };

   r.add(
      "std::function + lambda -> method",
      iterations,
      1,
      [iterations, &abc, &lam0]()
      {
         auto c = iterations;
         std::size_t current = 0;
         auto count = abc.size();
         while (c--)
         {
            auto o = abc[current].get();
            lam0(o, c);
            if (++current == count)
               current = 0;
         }
      }
   );

   std::function<Type(A*,Type)> lam1 =
   [](A* a, Type v)
   {
      return a->virtualCall(v);
   };

   r.add(
      "std::function + lambda -> virtual method",
      iterations,
      1,
      [iterations, &abc, &lam1]()
      {
         auto c = iterations;
         std::size_t current = 0;
         auto count = abc.size();
         while (c--)
         {
            auto o = abc[current].get();
            lam1(o, c);
            if (++current == count)
               current = 0;
         }
      }
   );

   r.run(std::cout);

   return 0;
}

