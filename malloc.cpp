#include <cstdlib>
#include <vector>

#include "benchmark/runner.hpp"
#include "benchmark/util.hpp"


namespace
{


struct Fixture
   : public Benchmark::Fixture
{
   Fixture(
      std::size_t allocCount,
      std::initializer_list<std::size_t> sizes
   )
      : kSizes(sizes)
      , m_allocCount(allocCount)
   {
   }

   void initialize(unsigned threads) override
   {
      m_td.reserve(kSizes.size());
      for (unsigned i = 0; i < threads; ++i)
      {
         m_td.emplace_back(
            new ThreadData(m_allocCount)
         );
      }
   }

   void finalize() override
   {
      m_td.clear();
   }

protected:
   struct ThreadData
   {
      ThreadData(std::size_t allocs)
      {
         allocated.resize(allocs);
      }

      std::size_t nextSize = 0;
      std::size_t nextAlloc = 0;
      std::vector<void*> allocated;
   };

   std::vector<std::size_t> const kSizes;
   std::size_t const m_allocCount;
   std::vector<std::unique_ptr<ThreadData>> m_td;
};


struct Malloc
   : public Fixture
{
   Malloc(
      std::size_t allocCount,
      std::initializer_list<std::size_t> sizes
   )
      : Fixture(allocCount, sizes)
   {
   }

   Benchmark::Counter run(
      Benchmark::Counter iterations,
      Benchmark::Tid tid
   ) override
   {
      std::size_t const n = std::min(
         std::size_t{iterations},
         m_allocCount
      );

      auto td = m_td[tid].get();
      for (std::size_t i = 0; i < n; ++i)
      {
         auto iSize = td->nextSize++;
         if (td->nextSize == kSizes.size())
            td->nextSize = 0;

         auto nextSize = kSizes[iSize];
         td->allocated[i] = std::malloc(nextSize);
      }

      return iterations - n;
   }

   void epilogue(Benchmark::Tid tid) override
   {
      auto td = m_td[tid].get();
      for (auto& a: td->allocated)
      {
         std::free(a);
         a = nullptr;
      }
   }

};

} // namespace


int main(int argc, char** argv)
{
   auto iterations = Benchmark::getIntArgOr(
      "-n",
      1000000ULL,
      argc,
      argv
   );

   if (iterations < 1)
   {
      std::cerr << "-n must be positive\n";
      return -1;
   }

   Benchmark::Runner r("malloc/free speed", iterations);

   r.add(
      "malloc()",
      Fixture::make<Malloc>(
         std::size_t{1024 * 1024},
         std::initializer_list<std::size_t>
            { 1, 3, 7, 10, 23, 65, 145, 277, 419, 1023 }
      ),
      { 1, 2, 4, 8 }
   );

   r.run();

   return 0;
}
