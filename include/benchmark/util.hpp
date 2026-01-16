#pragma once


#include <cassert>
#include <charconv>
#include <concepts>
#include <memory>
#include <optional>
#include <system_error>
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


inline std::optional<int> getIntArg(
   std::string_view name,
   int argc,
   char** argv
)
{
   for (int i = 1; i < argc; ++i)
   {
      auto len = std::strlen(argv[i]);
      std::string_view v{ argv[i], len };
      if (v == name)
      {
         if (i + 1 < argc)
         {
            auto str2 = argv[i + 1];
            auto len2 = std::strlen(str2);
            int result = 0;
            auto parsed = std::from_chars(
               str2,
               str2 + len2,
               result
            );
            if (parsed.ec == std::errc{})
               return result;
            else
               break;
         }
      }
   }

   return std::nullopt;
}

inline int getIntArgOr(
   std::string_view name,
   int alt,
   int argc,
   char** argv
)
{
   auto r = getIntArg(name, argc, argv);
   if (r)
      return *r;

   return alt;
}


} // namespace
