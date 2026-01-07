#pragma once


#include <cassert>
#include <concepts>
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


template <typename T>
concept BoolSource = std::is_invocable_r_v<bool, T>;


template <class IBase, class Derived, class... Others>
   requires std::is_base_of_v<IBase, Derived> &&
   (std::is_base_of_v<IBase, Others> && ...)
struct AnyObject;


template <class IBase, class Derived>
   requires std::is_base_of_v<IBase, Derived>
struct AnyObject<IBase, Derived>
{
   constexpr AnyObject() noexcept
   {}

   void push(BoolSource auto const& selector, auto&&... args)
   {
      if (selector())
         this->m_objects.emplace_back(
            new Derived(
               std::forward<decltype(args)>(args)...
            )
         );
   }

   IBase* one() noexcept
   {
      assert(!m_objects.empty());
      if (m_next >= m_objects.size())
         m_next = 0;

      return m_objects[m_next++].get();
   }

   auto size() const noexcept
   {
      return m_objects.size();
   }

   void clear() noexcept
   {
      m_objects.clear();
   }

   IBase* at(std::size_t index) noexcept
   {
      assert(index < m_objects.size());
      return m_objects[index].get();
   }

   auto& objects() noexcept
   {
      return m_objects;
   }

protected:
   std::vector<std::unique_ptr<IBase>> m_objects;
   std::size_t m_next = 0;
};


template <class IBase, class Derived, class... Others>
   requires std::is_base_of_v<IBase, Derived> &&
   (std::is_base_of_v<IBase, Others> && ...)
struct AnyObject
   : public AnyObject<IBase, Others...>
{
   using Super = AnyObject<IBase, Others...>;

   constexpr AnyObject() noexcept = default;

   void push(BoolSource auto const& selector, auto&&... args)
   {
      if (selector())
         this->m_objects.emplace_back(
            new Derived(
               std::forward<decltype(args)>(args)...
            )
         );
      else
         Super::push(
            selector,
            std::forward<decltype(args)>(args)...
         );
   }

   void fill(
      BoolSource auto const& selector,
      std::size_t n,
      auto&&... args
   )
   {
      while (n--)
      {
         push(
            selector,
            std::forward<decltype(args)>(args)...
         );
      }
   }
};

} // namespace
