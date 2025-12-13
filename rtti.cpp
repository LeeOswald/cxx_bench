#include <cstdio>
#include <iostream>
#include <vector>
#include <typeinfo>

#include "benchmark.hpp"


namespace
{


class Base
{
public:
   static constexpr int TBase = 0;

   virtual ~Base() = default;

   constexpr Base(
      int id = -1,
      int type = TBase
   ) noexcept
      : m_id(id)
      , m_type(type)
   {
   }

   constexpr auto id() const noexcept
   {
       return m_id;
   }

   constexpr auto type() const noexcept
   {
      return m_type;
   }

   virtual void say() = 0;

private:
   int m_id;
   int m_type;
};


class A: virtual public Base
{
public:
   static constexpr int TA = 1;

   A(int id, int type = TA) noexcept
      : Base(id, type) 
   {
   }

   void say() override
   {
      std::cout << "A::say(" << id() << ")\n";
   }
};


class B: virtual public Base
{
public:
   static constexpr int TB = 2;

   B(int id, int type = TB) noexcept
      : Base(id, type)
   {
   }

   void say() override
   {
      std::cout << "B::say(" << id() << ")\n";
   }
};


class AB: public A, public B
{
public:
   static constexpr int TAB = 3;

   AB(int id, int type = TAB) noexcept
      : A(id, type)
      , B(id, type)
   {
   }

   void say() override
   {
      std::cout << "AB::say(" 
                << A::id() << ":"
                << B::id() << ")\n";
   }
};


class BA: public B, public A
{
public:
   static constexpr int TBA = 3;

   BA(int id, int type = TBA) noexcept
      : B(id, type)
      , A(id, type)
   {
   }

   void say() override
   {
      std::cout << "BA::say("
                << A::id() << ":"
                << B::id() << ")\n";
   }
};


std::vector<std::unique_ptr<Base>> makeAB(std::size_t n)
{
   std::vector<std::unique_ptr<Base>> v;
   v.reserve(n);

   Benchmark::Random r(0);

   for (std::size_t i = 0; i < n; ++i)
   {
      if (r() % 2 == 0)
         v.emplace_back(new AB(i));
      else
         v.emplace_back(new BA(i));
   }

   return v;
}


} // namespace


int main()
{
   Benchmark::Runner r("RTTI performance", 100000000ULL);

   auto abv = makeAB(1024);

   r.add(
      "dynamic_cast",
      1,
      [&abv](std::uint64_t iterations)
      {
         auto c = iterations;
         std::size_t current = 0;
         auto count = abv.size();
         while (c--)
         {
            auto o = abv[current].get();
            auto ab = dynamic_cast<AB*>(o);
            if (!ab)
            {
               auto ba = dynamic_cast<BA*>(o);
               if (!ba)
               {
                  std::terminate();
               }
            }

            if (++current == count)
               current = 0;
         }
      }
   );

   r.add(
      "typeinfo",
      1,
      [&abv](std::uint64_t iterations)
      {
         auto c = iterations;
         std::size_t current = 0;
         auto count = abv.size();
         while (c--)
         {
            auto o = abv[current].get();

            auto& ty = typeid(*o);
            if (ty != typeid(AB))
               if (ty != typeid(BA))
                  std::terminate();

            if (++current == count)
               current = 0;
         }
      }
   );

   r.run(std::cout);

   return 0;
}
