#pragma once

#include <benchmark/cmdline.hpp>

#include <cstdlib>
#include <iostream>
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


template <class IBase>
using AnyObjectVector = std::vector<
   std::unique_ptr<IBase>
>;


template <class IBase, class Derived, class... Others>
   requires std::is_base_of_v<IBase, Derived> &&
   (std::is_base_of_v<IBase, Others> && ...)
struct AnyObject;


template <class IBase, class Derived>
   requires std::is_base_of_v<IBase, Derived>
struct AnyObject<IBase, Derived>
{
   static std::unique_ptr<IBase> make(
      [[maybe_unused]] BoolSource auto const& selector,
      auto&&... args
   )
   {
      return std::make_unique<Derived>(
         std::forward<decltype(args)>(args)...
      );
   }

   static void push(
      AnyObjectVector<IBase>& v,
      [[maybe_unused]] BoolSource auto const& selector,
      auto&&... args
   )
   {
      v.emplace_back(
         new Derived(
            std::forward<decltype(args)>(args)...
         )
      );
   }
};


template <class IBase, class Derived, class... Others>
   requires std::is_base_of_v<IBase, Derived> &&
   (std::is_base_of_v<IBase, Others> && ...)
struct AnyObject
   : public AnyObject<IBase, Others...>
{
   using Super = AnyObject<IBase, Others...>;

   static std::unique_ptr<IBase> make(
      BoolSource auto const& selector,
      auto&&... args
   )
   {
      if (selector())
         return std::make_unique<Derived>(
            std::forward<decltype(args)>(args)...
         );
      else
         return Super::make(
            selector,
            std::forward<decltype(args)>(args)...
         );
   }

   static void push(
      AnyObjectVector<IBase>& v,
      BoolSource auto const& selector,
      auto&&... args
   )
   {
      if (selector())
         v.emplace_back(
            new Derived(
               std::forward<decltype(args)>(args)...
            )
         );
      else
         Super::push(
            v,
            selector,
            std::forward<decltype(args)>(args)...
         );
   }

   static void fill(
      AnyObjectVector<IBase>& v,
      BoolSource auto const& selector,
      std::size_t n,
      auto&&... args
   )
   {
      while (n--)
      {
         push(
            v,
            selector,
            std::forward<decltype(args)>(args)...
         );
      }
   }
};


template <typename T>
   requires std::is_integral_v<T>
bool bindArg(
   CmdLine& cmd,
   std::string_view name,
   T& var,
   std::string_view help
)
{
   auto res = cmd.get(name, var);
   if (res == CmdLine::ArgType::Ok)
      return true;

   if (res == CmdLine::ArgType::NotFound)
      return false;

   std::cerr << help << "\n";
   std::exit(EXIT_FAILURE);
}


} // namespace
