#pragma once


namespace Benchmark
{


template <class Provider>
class Stopwatch
{
public:
   using Value = typename Provider::Value;

   constexpr Stopwatch(Provider pr = {}) noexcept
      : m_provider(pr)
   {}

   void start() noexcept
   {
      m_started = m_provider();
   }

   Value stop() noexcept
   {
      auto delta = m_provider() - m_started;
      m_elapsed += delta;
      return delta;
   }

   Value value() const noexcept
   {
      return m_elapsed;
   }

private:
   Provider m_provider;
   Value m_started = {};
   Value m_elapsed = {};
};



} // namespace
