
#include "calls_impl.hpp"
#include "benchmark/random.hpp"


namespace Calls
{

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

struct A::Impl
{
   Type id;

   Impl(Type id)
      : id(id)
   {}

   Type call(Type v)
   {
      id += v;
      return id;
   }
};

A::~A()
{
}

A::A(Type v) noexcept
      : m_v(v)
      , m_impl(new Impl(v))
{
}

Type A::methodCall(Type a)
{
   m_v += a;
   return m_v;
}

Type A::pimplCall(Type v)
{
   return m_impl->call(v);
}


Type B::virtualCall(Type a)
{
   m_v += a;
   return m_v;
}

Type B::pseudoVirtualImpl(
   A* this_,
   Type a
)
{
   this_->m_v += a;
   return this_->m_v;
}


Type C::virtualCall(Type a)
{
   m_v += a;
   return m_v;
}

Type C::pseudoVirtualImpl(
   A* this_,
   Type a
)
{
   this_->m_v += a;
   return this_->m_v;
}


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


void run()
{
   constexpr std::size_t iterations = 1000000000ULL;

   Benchmark::Runner r("Function call speed", iterations);

   auto ff = makeFreeFun(1024);

   r.add(
      "free function",
      1,
      [&ff](std::uint64_t iterations)
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
      1,
      [&abc](std::uint64_t iterations)
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
      1,
      [&abc](std::uint64_t iterations)
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
      1,
      [&abc](std::uint64_t iterations)
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

   r.add(
      "pImpl method",
      1,
      [&abc](std::uint64_t iterations)
      {
         auto c = iterations;
         std::size_t current = 0;
         auto count = abc.size();
         while (c--)
         {
            abc[current].get()->pimplCall(c);
            if (++current == count)
               current = 0;
         }
      }
   );

   std::function<Type(FreeData*,Type)> fun = freeFun;

   r.add(
      "std::function -> free function",
      1,
      [&ff, &fun](std::uint64_t iterations)
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
      1,
      [&abc, &fun0](std::uint64_t iterations)
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
      1,
      [&abc, &fun1](std::uint64_t iterations)
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
      1,
      [&abc, &lam0](std::uint64_t iterations)
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
      1,
      [&abc, &lam1](std::uint64_t iterations)
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

   r.run();
}


} // namespace

