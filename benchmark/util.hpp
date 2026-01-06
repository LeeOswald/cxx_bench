#pragma once


#include "random.hpp"

#include <cassert>
#include <memory>
#include <vector>


#if BM_POSIX
   #include <unistd.h>
#endif

#if BM_WINDOWS
   #include <windows.h>
#endif



namespace Benchmark
{

inline std::uint64_t tid() noexcept
{
#if BM_POSIX
   return ::gettid();
#elif BM_WINDOWS
   return ::GetCurrentThreadId();
#else
   static_assert(false, "unsupported platform");
#endif
}


template <class IBase, class Derived, class... Others>
struct AnyObject;


template <class IBase, class Derived>
struct AnyObject<IBase, Derived>
{
   constexpr AnyObject(Random& r) noexcept
      : _rand(r)
   {}

   template <typename... Args>
   void push(Args&&... args)
   {
      if (this->_rand() % 2 == 0)
         this->objects.emplace_back(
            new Derived(std::forward<Args>(args)...)
         );
   }

   IBase* one() noexcept
   {
      assert(!objects.empty());
      auto index = _rand() % objects.size();
      return objects[index].get();
   }

   void clear() noexcept
   {
      objects.clear();
   }

   std::vector<std::unique_ptr<IBase>> objects;

protected:
   Random& _rand;
};


template <class IBase, class Derived, class... Other>
struct AnyObject
   : public AnyObject<IBase, Other...>
{
   using Super = AnyObject<IBase, Other...>;

   constexpr AnyObject(Random& r) noexcept
      : Super(r)
   {}

   template <typename... Args>
   void push(Args&&... args)
   {
      if (this->_rand() % 2 == 0)
         this->objects.emplace_back(
            new Derived(std::forward<Args>(args)...)
         );
      else
         Super::push(std::forward<Args>(args)...);
   }

   template <typename... Args>
   void fill(std::size_t n, Args&&... args)
   {
      while (n--)
      {
         push(std::forward<Args>(args)...);
      }
   }
};

} // namespace
