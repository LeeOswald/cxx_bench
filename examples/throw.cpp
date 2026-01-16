#include <algorithm>
#include <atomic>
#include <exception>

#include "benchmark/benchmark.hpp"
#include "benchmark/runner.hpp"


namespace
{


class Fixture :
   public Benchmark::Fixture
{
public:
   Fixture(unsigned maxDepth, unsigned throwAtDepth)
      : m_maxDepth(maxDepth)
      , m_throwAtDepth(throwAtDepth)
   {}

   void finalize() override
   {
      g_banged += t_banged;
      g_frames += t_frames;
   }


protected:
   BM_NOINLINE void frame(unsigned level)
   {
      Frame f;

      if (level == m_throwAtDepth)
         throw Bang(level);

      if (level + 1 <= m_maxDepth)
         frame(level + 1);
   }

   struct Frame
   {
      ~Frame()
      {
         --t_frames;
      }

      Frame()
      {
         ++t_frames;
      }
   };

   struct Bang
   {
      Bang(unsigned l)
         : level(l)
      {
         ++Fixture::t_banged;
      }

      unsigned level;
   };

   static std::atomic<long> g_banged;
   static thread_local long t_banged;
   static std::atomic<long> g_frames;
   static thread_local long t_frames;

   unsigned const m_maxDepth;
   unsigned const m_throwAtDepth;
};

thread_local long Fixture::t_banged = 0;
thread_local long Fixture::t_frames = 0;
std::atomic<long> Fixture::g_banged = 0;
std::atomic<long> Fixture::g_frames = 0;


class MaybeTryCatch
   : public Fixture
{
public:
   MaybeTryCatch(unsigned maxDepth, unsigned throwAtDepth)
      : Fixture(maxDepth, throwAtDepth)
   {}

   void finalize() override
   {
      Fixture::finalize();
      g_caught += t_caught;
   }

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid
   ) override
   {
      if (m_throwAtDepth == unsigned(-1))
         notw(iterations);
      else
         tw(iterations);

      return 0;
   }

private:
   BM_NOINLINE void tw_impl()
   {
      try
      {
         frame(0);
      }
      catch (Bang&)
      {
         ++t_caught;
      }
   }

   BM_NOINLINE void tw(Benchmark::Counter iterations)
   {
      while (iterations--)
         tw_impl();
   }

   BM_NOINLINE void notw(Benchmark::Counter iterations)
   {
      while (iterations--)
         frame(0);
   }

   static std::atomic<long> g_caught;
   static thread_local long t_caught;
};

thread_local long MaybeTryCatch::t_caught = 0;
std::atomic<long> MaybeTryCatch::g_caught = 0;

} // namespace


int main()
{
   constexpr unsigned maxDepth = 16;

   constexpr std::uint64_t iterations = 100000ULL;
   Benchmark::Runner r("Exception performance", iterations);

   r.add(
      "baseline (no try/catch)",
      Fixture::make<MaybeTryCatch>(maxDepth - 1, unsigned(-1))
   );

   r.add(
      "try/catch, don't throw",
      Fixture::make<MaybeTryCatch>(maxDepth - 1, maxDepth),
      { 1, 2, 4 }
   );

   r.add(
      "try/catch + throw",
      Fixture::make<MaybeTryCatch>(maxDepth, maxDepth - 1),
      { 1, 2, 4 }
   );

   r.run();

   return 0;
}
