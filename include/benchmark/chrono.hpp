#pragma once

#include <chrono>



namespace Benchmark
{


template <typename Duration>
constexpr std::uint64_t ns(Duration v) noexcept
{
   return std::chrono::duration_cast<
      std::chrono::nanoseconds
   >(v).count();
}

template <typename Duration>
constexpr std::uint64_t us(Duration v) noexcept
{
   return std::chrono::duration_cast<
      std::chrono::microseconds
   >(v).count();
}

template <typename Duration>
constexpr std::uint64_t ms(Duration v) noexcept
{
   return std::chrono::duration_cast<
      std::chrono::milliseconds
   >(v).count();
}


} // namespace
