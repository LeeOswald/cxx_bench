#include <functional>
#include <memory>
#include <vector>


#include "benchmark/compiler.hpp"
#include "benchmark/runner.hpp"
#include "benchmark/util.hpp"


namespace
{


class Fixture
   : public Benchmark::Fixture
{
public:
   Fixture() noexcept
      : m_objs(rand())
   {
   }

   void initialize(unsigned) override
   {
      m_objs.fill(64, rand());
   }

   void finalize() override
   {
      m_objs.clear();
   }

protected:
   using Value = std::size_t;

   static volatile Value g_dontOptimize;

   static Benchmark::Random& rand() noexcept
   {
      static Benchmark::Random r(Benchmark::tid());
      return r;
   }

   struct IObj
   {
      IObj(Benchmark::Random& r)
         : v(r())
      {}

      virtual ~IObj() = default;
      virtual void virtualMethod(Value a, Value b) noexcept = 0;

      inline void inlineMethod(Value a, Value b) noexcept
      {
         v += (a + b);
      }

      BM_NOINLINE void classMethod(Value a, Value b) noexcept
      {
         v += (a + b);
      }

      BM_NOINLINE static void staticMethod(IObj* o, Value a, Value b) noexcept
      {
         o->v += (a + b);
      }

      Value v;
   };

   struct A : public IObj
   {
      A(Benchmark::Random& r)
         : IObj(r)
      {
      }

      BM_NOINLINE void virtualMethod(Value a, Value b) noexcept override
      {
         v += (a + b);
      }
   };

   struct B : public IObj
   {
      B(Benchmark::Random& r)
         : IObj(r)
      {
      }

      BM_NOINLINE void virtualMethod(Value a, Value b) noexcept override
      {
         v += (a - b);
      }
   };

   struct C : public IObj
   {
      C(Benchmark::Random& r)
         : IObj(r)
      {
      }

      BM_NOINLINE void virtualMethod(Value a, Value b) noexcept override
      {
         v -= (a + b);
      }
   };

   struct D : public IObj
   {
      D(Benchmark::Random& r)
         : IObj(r)
      {
      }

      BM_NOINLINE void virtualMethod(Value a, Value b) noexcept override
      {
         v -= (a - b);
      }
   };

   Benchmark::AnyObject<
      IObj, A, B, C, D
   > m_objs;
};


volatile Fixture::Value Fixture::g_dontOptimize = 0;


class StaticMethod final
   : public Fixture
{
public:
   StaticMethod() noexcept = default;

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid
   ) override
   {
      while (iterations--)
      {
         auto o = m_objs.one();
         IObj::staticMethod(o, iterations, iterations);
      }

      g_dontOptimize = m_objs.one()->v;
      return 0;
   }
};


class InlineMethod final
   : public Fixture
{
public:
   InlineMethod() noexcept = default;

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid
   ) override
   {
      while (iterations--)
      {
         auto o = m_objs.one();
         o->inlineMethod(iterations, iterations);
      }

      g_dontOptimize = m_objs.one()->v;
      return 0;
   }
};


class ClassMethod final
   : public Fixture
{
public:
   ClassMethod() noexcept = default;

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid
   ) override
   {
      while (iterations--)
      {
         auto o = m_objs.one();
         o->classMethod(iterations, iterations);
      }

      g_dontOptimize = m_objs.one()->v;
      return 0;
   }
};


class VirtualMethod
   : public Fixture
{
public:
   VirtualMethod() noexcept = default;

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid
   ) override
   {
      while (iterations--)
      {
         auto o = m_objs.one();
         o->virtualMethod(iterations, iterations);
      }

      g_dontOptimize = m_objs.one()->v;
      return 0;
   }

private:

};


} // namespace



int main()
{
   constexpr std::size_t iterations = 100000000ULL;

   Benchmark::Runner r("Function call speed", iterations);

   r.add(
      "inline method",
      Fixture::make<InlineMethod>()
   );

   r.add(
      "static method",
      Fixture::make<StaticMethod>()
   );

   r.add(
      "regular method",
      Fixture::make<ClassMethod>()
   );

   r.add(
      "virtual method",
      Fixture::make<VirtualMethod>()
   );

#if 0
   auto ff = makeFreeFun(1024);

   r.add(
      "free function",
      [&ff](Benchmark::Counter iterations, Benchmark::Tid)
         -> Benchmark::Counter
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

         return 0;
      }
   );

   auto abc = makeABC(1024);

   r.add(
      "class  method",
      [&abc](Benchmark::Counter iterations, Benchmark::Tid)
         -> Benchmark::Counter
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

         return 0;
      }
   );

   r.add(
      "virtual method",
      [&abc](Benchmark::Counter iterations, Benchmark::Tid)
         -> Benchmark::Counter
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

         return 0;
      }
   );

   r.add(
      "pseudo-virtual method",
      [&abc](Benchmark::Counter iterations, Benchmark::Tid)
         -> Benchmark::Counter
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

         return 0;
      }
   );

   r.add(
      "pImpl method",
      [&abc](Benchmark::Counter iterations, Benchmark::Tid)
         -> Benchmark::Counter
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

         return 0;
      }
   );

   std::function<Type(FreeData*,Type)> fun = freeFun;

   r.add(
      "std::function -> free function",
      [&ff, &fun](Benchmark::Counter iterations, Benchmark::Tid)
         -> Benchmark::Counter
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

         return 0;
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
      [&abc, &fun0](Benchmark::Counter iterations, Benchmark::Tid)
         -> Benchmark::Counter
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

         return 0;
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
      [&abc, &fun1](Benchmark::Counter iterations, Benchmark::Tid)
         -> Benchmark::Counter
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

         return 0;
      }
   );

   std::function<Type(A*,Type)> lam0 =
   [](A* a, Type v)
   {
      return a->methodCall(v);
   };

   r.add(
      "std::function + lambda -> method",
      [&abc, &lam0](Benchmark::Counter iterations, Benchmark::Tid)
         -> Benchmark::Counter
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

         return 0;
      }
   );

   std::function<Type(A*,Type)> lam1 =
   [](A* a, Type v)
   {
      return a->virtualCall(v);
   };

   r.add(
      "std::function + lambda -> virtual method",
      [&abc, &lam1](Benchmark::Counter iterations, Benchmark::Tid)
         -> Benchmark::Counter
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

         return 0;
      }
   );
#endif

   r.run();

   return 0;
}

