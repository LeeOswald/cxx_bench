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
      AnyObject::fill(
         m_objs,
         [this]() { return rand()() % 3 == 0; },
         256,
         rand()
      );
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
         v += (a + b + 1);
      }

      BM_NOINLINE void classMethod(Value a, Value b) noexcept
      {
         v += (a + b + 1);
      }

      BM_NOINLINE static void staticMethod(IObj* o, Value a, Value b) noexcept
      {
         o->v += (a + b + 1);
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
         v += (a + b + 2);
      }

      BM_NOINLINE static void staticMethod2(IObj* o, Value a, Value b) noexcept
      {
         o->v += (a + b + 2);
      }

      BM_NOINLINE void classMethod2(
         Value a,
         Value b
      ) noexcept
      {
         v += (a + b + 2);
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
         v += (a + b + 3);
      }

      BM_NOINLINE static void staticMethod2(IObj* o, Value a, Value b) noexcept
      {
         o->v += (a + b + 3);
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
         v += (a + b + 3);
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

   struct C : public IObj
   {
      C(Benchmark::Random& r)
         : IObj(r)
      {
      }

      BM_NOINLINE void virtualMethod(Value a, Value b) noexcept override
      {
         v += (a + b + 3);
      }

      BM_NOINLINE static void staticMethod2(IObj* o, Value a, Value b) noexcept
      {
         o->v += (a + b + 3);
      }

      Fn makeStaticFn() noexcept override
      {
         return { &C::staticMethod2 };
      }

      BM_NOINLINE void classMethod2(
         Value a,
         Value b
      ) noexcept
      {
         v += (a + b + 3);
      }

      MemberFn bindMemberFn() noexcept override
      {
         return std::bind(
            &C::classMethod2,
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

   struct D : public IObj
   {
      D(Benchmark::Random& r)
         : IObj(r)
      {
      }

      BM_NOINLINE void virtualMethod(Value a, Value b) noexcept override
      {
         v += (a + b + 4);
      }

      BM_NOINLINE static void staticMethod2(IObj* o, Value a, Value b) noexcept
      {
         o->v += (a + b + 4);
      }

      Fn makeStaticFn() noexcept override
      {
         return { &D::staticMethod2 };
      }

      BM_NOINLINE void classMethod2(
         Value a,
         Value b
      ) noexcept
      {
         v += (a + b + 4);
      }

      MemberFn bindMemberFn() noexcept override
      {
         return std::bind(
            &D::classMethod2,
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

   IObj* one() noexcept
   {
      assert(!m_objs.empty());
      auto idx = m_next++;
      if (m_next == m_objs.size())
         m_next = 0;

      return m_objs[idx].get();
   }

   IObj* at(std::size_t idx) noexcept
   {
      assert(idx < m_objs.size());
      return m_objs[idx].get();
   }

   using AnyObject = Benchmark::AnyObject<
      IObj, A, B, C, D
   >;

   Benchmark::AnyObjectVector<IObj> m_objs;
   std::size_t m_next = 0;
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
         auto o = one();
         IObj::staticMethod(o, iterations, iterations);
      }

      g_dontOptimize = one()->v;
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
         auto o = one();
         o->inlineMethod(iterations, iterations);
      }

      g_dontOptimize = one()->v;
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
         auto o = one();
         o->classMethod(iterations, iterations);
      }

      g_dontOptimize = one()->v;
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
         auto o = one();
         o->virtualMethod(iterations, iterations);
      }

      g_dontOptimize = one()->v;
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

      for (auto& o: m_objs)
         m_outers.emplace_back(
            new Outer(o.get())
         );
   }

   void finalize() override
   {
      m_outers.clear();

      Fixture::finalize();
   }

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid
   ) override
   {
      while (iterations--)
      {
         auto o = oneOu();

         o->method(iterations, iterations);
      }

      g_dontOptimize = one()->v;
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

   Outer* oneOu() noexcept
   {
      auto idx = m_nextOu++;
      if (m_nextOu == m_outers.size())
        m_nextOu = 0;

      return m_outers[idx].get();
   }

   std::vector<std::unique_ptr<Outer>> m_outers;
   std::size_t m_nextOu = 0;
};


class StdFunctionStaticMethod
   : public Fixture
{
public:
   StdFunctionStaticMethod() noexcept = default;

   void initialize(unsigned tid) override
   {
      Fixture::initialize(tid);

      for (auto& o: m_objs)
         m_fns.push_back(
            o->makeStaticFn()
         );
   }

   void finalize() override
   {
      m_fns.clear();

      Fixture::finalize();
   }

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid
   ) override
   {
      auto n = m_fns.size();

      while (iterations--)
      {
         auto index = rand()() % n;
         auto o = at(index);
         m_fns[index](o, iterations, iterations);
      }

      g_dontOptimize = one()->v;
      return 0;
   }

private:
   std::vector<IObj::Fn> m_fns;
};


class StdFunctionToMethod
   : public Fixture
{
public:
   StdFunctionToMethod() noexcept = default;

   void finalize() override
   {
      m_fns.clear();

      Fixture::finalize();
   }

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid
   ) override
   {
      auto n = m_fns.size();

      while (iterations--)
      {
         auto index = rand()() % n;
         auto o = at(index);
         m_fns[index](iterations, iterations);
      }

      g_dontOptimize = one()->v;
      return 0;
   }

protected:
   std::vector<IObj::MemberFn> m_fns;
};


class StdFunctionBindClassMethod
   : public StdFunctionToMethod
{
public:
   StdFunctionBindClassMethod() noexcept = default;

   void initialize(unsigned tid) override
   {
      Fixture::initialize(tid);

      for (auto& o: m_objs)
         m_fns.push_back(
            o->bindMemberFn()
         );
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

      for (auto& o: m_objs)
         m_fns.push_back(
            o->lambdaMemberFn()
         );
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

      for (auto& o: m_objs)
         m_fns.push_back(
            o->bindVirtualMemberFn()
         );
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

      for (auto& o: m_objs)
         m_fns.push_back(
            o->lambdaVirtualMemberFn()
         );
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

