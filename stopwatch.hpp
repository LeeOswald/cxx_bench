#pragma once


namespace Benchmark
{


template <class Provider>
class Stopwatch
{
public:
   using Unit = typename Provider::Unit;

   constexpr Stopwatch(Provider pr = {})
      : m_provider(pr)
   {}

   void start() noexcept
   {
      m_started = m_provider();
   }

   Unit stop() noexcept
   {
      auto delta = m_provider() - m_started;
      m_elapsed += delta;
      return delta;
   }

   Unit value() const noexcept
   {
      return m_elapsed;
   }

private:
   Provider m_provider;
   Unit m_started = {};
   Unit m_elapsed = {};
};



} // namespace
