#pragma once

#include <cstdint>


namespace Benchmark
{

// a weak but fast PRNG
class Random final
{
private:
   std::uint64_t m_state;

public:
   constexpr Random(std::uint64_t seed) noexcept
      : m_state((seed << 1) | 1)
   {}

   std::uint64_t operator()() noexcept
   {
      auto x = m_state;
      x ^= x >> 12;
      x ^= x << 25;
      x ^= x >> 27;
      m_state = x;
      return x * 0x2545F4914F6CDD1DULL;
   }

   template <typename T>
   T operator()(T max) noexcept
   {
      return static_cast<T>(
         (operator()()) % (max + 1)
      );
   }
};



} // namespace
