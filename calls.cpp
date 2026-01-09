#include <functional>
#include <memory>
#include <vector>


#include "benchmark/compiler.hpp"
#include "benchmark/random.hpp"
#include "benchmark/runner.hpp"
#include "benchmark/util.hpp"


namespace
{


class Fixture
   : public Benchmark::Fixture
{
public:
   Fixture() noexcept = default;

   void initialize(unsigned) override
   {
      auto o = Benchmark::AnyObject<IObj, A, B>::make(
         [this]() { return rand()() % 2 == 0; },
         rand()
      );

      m_obj.swap(o);
   }

   void finalize() override
   {
      m_obj.reset();
   }

protected:
   using Value = std::size_t;

   static Value heavyFun(Value x, Value y) noexcept
   {
      return x * 7 + x % (y / 3 + 1);
   }

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
         v += heavyFun(a, b);
      }

      BM_NOINLINE void classMethod(Value a, Value b) noexcept
      {
         v += heavyFun(a, b);
      }

      BM_NOINLINE static void staticMethod(IObj* o, Value a, Value b) noexcept
      {
         o->v += heavyFun(a, b);
      }

      using Fn = std::function<void(IObj*, Value, Value)>;
      virtual Fn makeStaticFn() noexcept = 0;

      using MemberFn = std::function<void(Value, Value)>;
      virtual MemberFn bindMemberFn() noexcept = 0;
      virtual MemberFn lambdaMemberFn() noexcept = 0;

      MemberFn bindVirtualMemberFn() noexcept
      {
         return std::bind(
            &IObj::virtualMethod,
            this,
            std::placeholders::_1,
            std::placeholders::_2
         );
      }

      MemberFn lambdaVirtualMemberFn() noexcept
      {
         return [this](Value a, Value b)
         {
            this->virtualMethod(a, b);
         };
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
         v -= heavyFun(a, b);
      }

      BM_NOINLINE static void staticMethod2(IObj* o, Value a, Value b) noexcept
      {
         o->v -= heavyFun(a, b);
      }

      BM_NOINLINE void classMethod2(
         Value a,
         Value b
      ) noexcept
      {
         v -= heavyFun(a, b);
      }

      MemberFn bindMemberFn() noexcept override
      {
         return std::bind(
            &A::classMethod2,
            this,
            std::placeholders::_1,
            std::placeholders::_2
         );
      }

      MemberFn lambdaMemberFn() noexcept override
      {
         return [this](Value a, Value b)
         {
            this->classMethod2(a, b);
         };
      }

      Fn makeStaticFn() noexcept override
      {
         return { &A::staticMethod2 };
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
         v += heavyFun(b, a);
      }

      BM_NOINLINE static void staticMethod2(IObj* o, Value a, Value b) noexcept
      {
         o->v += heavyFun(b, a);
      }

      Fn makeStaticFn() noexcept override
      {
         return { &B::staticMethod2 };
      }

      BM_NOINLINE void classMethod2(
         Value a,
         Value b
      ) noexcept
      {
         v += heavyFun(b, a);
      }

      MemberFn bindMemberFn() noexcept override
      {
         return std::bind(
            &B::classMethod2,
            this,
            std::placeholders::_1,
            std::placeholders::_2
         );
      }

      MemberFn lambdaMemberFn() noexcept override
      {
         return [this](Value a, Value b)
         {
            this->classMethod2(a, b);
         };
      }
   };

   std::unique_ptr<IObj> m_obj;
};


volatile Fixture::Value Fixture::g_dontOptimize = 0;


class StaticMethod final
   : public Fixture
{
public:
   StaticMethod() noexcept = default;

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid tid
   ) override
   {
      auto o = m_obj.get();
      while (iterations--)
      {
         IObj::staticMethod(o, iterations, tid);
      }

      g_dontOptimize = o->v;
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
      Benchmark::Tid tid
   ) override
   {
      auto o = m_obj.get();
      while (iterations--)
      {
         o->inlineMethod(iterations, tid);
      }

      g_dontOptimize = o->v;
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
      Benchmark::Tid tid
   ) override
   {
      auto o = m_obj.get();
      while (iterations--)
      {
         o->classMethod(iterations, tid);
      }

      g_dontOptimize = o->v;
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
      Benchmark::Tid tid
   ) override
   {
      auto o = m_obj.get();
      while (iterations--)
      {
         o->virtualMethod(iterations, tid);
      }

      g_dontOptimize = o->v;
      return 0;
   }
};


class PImplMethod final
   : public Fixture
{
public:
   PImplMethod() noexcept = default;

   void initialize(unsigned tid) override
   {
      Fixture::initialize(tid);
      m_outer.reset(new Outer(m_obj.get()));
   }

   void finalize() override
   {
      m_outer.reset();

      Fixture::finalize();
   }

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid tid
   ) override
   {
      auto o = m_outer.get();
      while (iterations--)
      {
         o->method(iterations, tid);
      }

      g_dontOptimize = m_obj->v;
      return 0;
   }

private:
   struct Outer
   {
      Outer(IObj* impl) noexcept
         : m_impl(impl)
      {}

      void method(Value a, Value b) noexcept
      {
         m_impl->classMethod(a, b);
      }

      IObj* m_impl;
   };

   std::unique_ptr<Outer> m_outer;
};


class StdFunctionStaticMethod
   : public Fixture
{
public:
   StdFunctionStaticMethod() noexcept = default;

   void initialize(unsigned tid) override
   {
      Fixture::initialize(tid);

      m_fn = m_obj->makeStaticFn();
   }

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid tid
   ) override
   {
      while (iterations--)
      {
         m_fn(m_obj.get(), iterations, tid);
      }

      g_dontOptimize = m_obj->v;
      return 0;
   }

private:
   IObj::Fn m_fn;
};


class StdFunctionToMethod
   : public Fixture
{
public:
   StdFunctionToMethod() noexcept = default;

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid tid
   ) override
   {
      while (iterations--)
      {
         m_fn(iterations, tid);
      }

      g_dontOptimize = m_obj->v;
      return 0;
   }

protected:
   IObj::MemberFn m_fn;
};


class StdFunctionBindClassMethod
   : public StdFunctionToMethod
{
public:
   StdFunctionBindClassMethod() noexcept = default;

   void initialize(unsigned tid) override
   {
      Fixture::initialize(tid);

      m_fn = m_obj->bindMemberFn();
   }
};


class StdFunctionLambdaClassMethod
   : public StdFunctionToMethod
{
public:
   StdFunctionLambdaClassMethod() noexcept = default;

   void initialize(unsigned tid) override
   {
      Fixture::initialize(tid);

      m_fn = m_obj->lambdaMemberFn();
   }
};

class StdFunctionBindVirtualMethod
   : public StdFunctionToMethod
{
public:
   StdFunctionBindVirtualMethod() noexcept = default;

   void initialize(unsigned tid) override
   {
      Fixture::initialize(tid);

      m_fn = m_obj->bindVirtualMemberFn();
   }
};


class StdFunctionLambdaVirtualMethod
   : public StdFunctionToMethod
{
public:
   StdFunctionLambdaVirtualMethod() noexcept = default;

   void initialize(unsigned tid) override
   {
      Fixture::initialize(tid);

      m_fn = m_obj->lambdaVirtualMemberFn();
   }
};

} // namespace


int main(int argc, char** argv)
{
   auto iterations = Benchmark::getIntArgOr(
      "-n",
      100000000ULL,
      argc,
      argv
   );

   if (iterations < 1)
   {
      std::cerr << "-n must be positive\n";
      return -1;
   }

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

   r.add(
      "p/impl method",
      Fixture::make<PImplMethod>()
   );

   r.add(
      "std::function -> static method",
      Fixture::make<StdFunctionStaticMethod>()
   );

   r.add(
      "std::function + std::bind -> regular method",
      Fixture::make<StdFunctionBindClassMethod>()
   );

   r.add(
      "std::function + lambda -> regular method",
      Fixture::make<StdFunctionLambdaClassMethod>()
   );

   r.add(
      "std::function + std::bind -> virtual method",
      Fixture::make<StdFunctionBindVirtualMethod>()
   );

   r.add(
      "std::function + lambda -> virtual method",
      Fixture::make<StdFunctionLambdaVirtualMethod>()
   );


   r.run();

   return 0;
}

