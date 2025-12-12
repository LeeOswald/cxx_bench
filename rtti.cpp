#include <cstdio>
#include <iostream>

#include "benchmark.hpp"


namespace
{


class Base
{
public:
   ~Base()
   {
      std::cout << static_cast<const void*>(this)
         << ".Base::~Base()\n";
   }

   Base(int id = -1)
      : m_id(id)
   {
      std::cout << static_cast<const void*>(this)
         << ".Base::Base()\n";
   }

   int id() const { return m_id; }

   virtual void say()
   {
      std::cout << "Base::say(" << id() << ")\n";
   }

private:
   int m_id;
};


class A: virtual public Base
{
public:
   ~A()
   {
      std::cout << static_cast<const void*>(this)
         << ".A::~A()\n";
   }

   A(int id) 
      : Base(id) 
   {
      std::cout << static_cast<const void*>(this)
         << ".A::A()\n";
   }

   void say() override
   {
      std::cout << "A::say(" << id() << ")\n";
   }
};


class B: virtual public Base
{
public:
   ~B()
   {
      std::cout << static_cast<const void*>(this)
         << ".B::~B()\n";
   }

   B(int id)
      : Base(id)
   {
      std::cout << static_cast<const void*>(this)
         << ".B::B()\n";
   }

   void say() override
   {
      std::cout << "B::say(" << id() << ")\n";
   }
};


class AB: public A, public B
{
public:
   ~AB()
   {
      std::cout << static_cast<const void*>(this)
         << ".AB::~AB()\n";
   }

   AB(int id)
      : A(1000+id)
      , B(2000+id)
   {
      std::cout << static_cast<const void*>(this)
         << ".AB::AB()\n";
   }

   void say() override
   {
      std::cout << "AB::say(" << id() << ")\n";
   }
};


BM_DONT_OPTIMIZE void bench(Base* o)
{
   auto a = dynamic_cast<AB*>(o);
   if (a)
      a->say();
   else
      std::cout << "Not an AB instance\n";
}


} // namedpace


int main()
{
   auto o = std::make_unique<AB>(13);
   bench(o.get());

   return 0;
}
