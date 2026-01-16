#pragma once

#include <charconv>
#include <cstring>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>


namespace Benchmark
{


class CmdLine final
{
public:
   CmdLine(int argc, char** argv)
   {
      std::string_view currentName;
      std::string_view currentVal;
      auto pushCurrent = [&]()
      {
         if (
             !currentName.empty() ||
             !currentVal.empty())
         {
            m_args.emplace_back(currentName, currentVal);
            currentName = {};
            currentVal = {};
         }
      };

      for (int i = 1; i < argc; ++i) // skip exe name
      {
         auto len = std::strlen(argv[i]);
         if (!len)
            continue;

         std::string_view cur(argv[i], len);
         if (cur[0] == '-')
         {
            // another named arg starts
            pushCurrent();
            currentName = cur;
         }
         else
         {
            if (!currentVal.empty())
               pushCurrent(); // previous unnamed value

            currentVal = cur;
         }
      }

      pushCurrent();
   }

   template <typename Fn>
      requires std::is_invocable_r_v<
         bool, Fn, std::string_view, std::string_view
      >
   void enumerate(Fn f) const
   {
      for (auto& a: m_args)
      {
         if (!f(a.first, a.second))
            break;
      }
   }

   template <typename Fn>
      requires std::is_invocable_r_v<
         bool, Fn, std::string_view, std::string_view
      >
   void enumerate(std::string_view name, Fn f) const
   {
      for (auto& a: m_args)
      {
         if (a.first == name)
         {
            if (!f(a.first, a.second))
               break;
         }
      }
   }

   enum class ArgType
   {
      NotFound,
      Invalid,
      Ok
   };

   ArgType get(
      std::string_view name,
      std::string_view& value
   ) const noexcept
   {
      auto it = std::find_if(
         m_args.begin(),
         m_args.end(),
         [name](Arg const& arg)
         {
            return (name == arg.first);
         }
      );

      if (it == m_args.end())
         return ArgType::NotFound;

      value = it->second;
      return ArgType::Ok;
   }

   bool contains(std::string_view name) const noexcept
   {
      std::string_view dummy;
      auto res = get(name, dummy);

      return (res != ArgType::NotFound);
   }

   template <typename T>
      requires std::is_integral_v<T>
   ArgType get(std::string_view name, T& value) const noexcept
   {
      std::string_view raw;
      auto res = get(name, raw);
      if (res != ArgType::Ok)
         return res;

      auto err = std::from_chars(
         raw.data(),
         raw.data() + raw.size(),
         value
      );

      if (err.ec == std::errc{})
         return ArgType::Ok;

      return ArgType::Invalid;
   }

private:
   using Arg = std::pair<std::string_view, std::string_view>;
   using Args = std::vector<Arg>;

   Args m_args;
};


} // namespace
