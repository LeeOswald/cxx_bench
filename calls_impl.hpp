#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "benchmark.hpp"


namespace Calls
{

using Type = std::size_t;


struct FreeData
{
   Type v;

   constexpr FreeData(Type v) noexcept
      : v(v)
   {}
};

Type freeFun(FreeData* this_, Type a) noexcept;


std::vector<std::unique_ptr<FreeData>>
   makeFreeFun(std::size_t n);


struct A
{
   virtual ~A();

   A(Type v) noexcept;

   Type methodCall(Type a);

   virtual Type virtualCall(Type a) = 0;

   Type (*pseudoVirtualCall)(A*, Type) = nullptr;

   Type m_v = 0;

   struct Impl;
   std::unique_ptr<Impl> m_impl;

   Type pimplCall(Type a);
};


struct B : public A
{
   Type virtualCall(Type a) override;

   static Type pseudoVirtualImpl(
      A* this_,
      Type a
   );

   B(Type v) noexcept
      : A(v)
   {
      A::pseudoVirtualCall = &B::pseudoVirtualImpl;
   }
};

struct C : public A
{
   Type virtualCall(Type a) override;

   static Type pseudoVirtualImpl(
      A* this_,
      Type a
   );

   C(Type v) noexcept
      : A(v)
   {
      A::pseudoVirtualCall = &C::pseudoVirtualImpl;
   }
};


std::vector<std::unique_ptr<A>>
   makeABC(std::size_t n);


void run();


} // namespace


