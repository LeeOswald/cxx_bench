
#include <typeinfo>

#include "benchmark/runner.hpp"
#include "benchmark/random.hpp"
#include "benchmark/util.hpp"

namespace
{


struct Fixture :
   public Benchmark::Fixture
{
   Fixture()
      : m_rand(Benchmark::tid())
   {}

   void initialize(unsigned) override
   {
      Benchmark::AnyObject<Base, AB, BA>::fill(
         m_objs,
         [this]() { return m_rand() % 2 == 0; },
         64
      );
   }

   void finalize() override
   {
      m_objs.clear();
      m_next = 0;
   }

protected:
   struct Base
   {
      virtual ~Base() = default;

      constexpr Base() noexcept
         : m_name("Base")
      {;
      }

      virtual const char* type() const noexcept = 0;

      const char* m_name;
   };

   struct A : virtual public Base
   {
      A() noexcept
         : m_name("A")
      {
      }

      const char* type() const noexcept override
      {
         return m_name;
      }

      const char* m_name;
   };

   struct B : virtual public Base
   {
      B() noexcept
         : m_name("B")
      {
      }


      const char* type() const noexcept override
      {
         return m_name;
      }

      const char* m_name;
   };

   struct AB : public A, public B
   {
      AB() noexcept
         : m_name("AB")
      {
      }

      const char* type() const noexcept override
      {
         return m_name;
      }

      const char* m_name;
   };

   struct BA : public B, public A
   {
      ~BA()
      {
      }

      BA() noexcept
         : m_name("BA")
      {
      }

      const char* type() const noexcept override
      {
         return m_name;
      }

      const char* m_name;
   };

   Base* one() noexcept
   {
      auto idx = m_next++;
      if (m_next == m_objs.size())
         m_next = 0;
      return m_objs[idx].get();
   }

   Benchmark::Random m_rand;
   Benchmark::AnyObjectVector<Base> m_objs;
   std::size_t m_next = 0;
   static volatile std::size_t g_dontOptimize;
};


volatile std::size_t Fixture::g_dontOptimize = 0;


struct DynamicCast
   : public Fixture
{
   DynamicCast() noexcept = default;

   Benchmark::Counter run(
      Benchmark::Counter n,
      Benchmark::Tid
   ) override
   {
      std::size_t k = 0;
      while (n--)
      {
         auto o = one();
         auto ab = dynamic_cast<AB*>(o);
         if (ab)
            ++k;
      }

      g_dontOptimize = k;
      return 0;
   }
};

struct StaticTypeId
   : public Fixture
{
   StaticTypeId() noexcept = default;

   Benchmark::Counter run(
      Benchmark::Counter n,
      Benchmark::Tid
   ) override
   {
      auto& base = typeid(Base);
      std::size_t nBase = 0;

      while (n--)
      {
         auto o = one();
         auto& ty = typeid(o);

         if (ty == base)
            ++nBase;
      }

      g_dontOptimize = nBase;
      return 0;
   }
};

struct DynamicTypeId
   : public Fixture
{
   DynamicTypeId() noexcept = default;

   Benchmark::Counter run(
      Benchmark::Counter n,
      Benchmark::Tid
   ) override
   {
      auto& ab = typeid(AB);
      std::size_t nAb = 0;

      while (n--)
      {
         auto o = one();
         auto& ty = typeid(*o);

         if (ty == ab)
            ++nAb;
      }

      g_dontOptimize = nAb;
      return 0;
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

   Benchmark::Runner r("RTTI performance", iterations);

   r.add(
      "static typeid()",
      Fixture::make<StaticTypeId>(),
      { 1, 2, 4 }
   );

   r.add(
      "dynamic typeid()",
      Fixture::make<DynamicTypeId>(),
      { 1, 2, 4 }
   );

   r.add(
      "dynamic_cast<>()",
      Fixture::make<DynamicCast>(),
      { 1, 2, 4 }
   );

   r.run();

   return 0;
}
