#include <benchmark/chrono.hpp>
#include <benchmark/runner.hpp>

#include <cmath>
#include <iomanip>


namespace Benchmark
{

void Runner::printCaption()
{
   m_console.line(out(), '=');

   out() << m_name;
   if (m_iterations > 0)
      out() << " (" << m_iterations << " iterations)";

   out() <<  std::endl;

   m_console.line(out(), '-');
}

void Runner::printFooter()
{
   m_console.line(out(), '-');
}

void Runner::printRunning(
   std::size_t index,
   std::size_t variant
)
{
   auto& bm = m_bm[index];

   if (variant == 0)
   {
      out() << "#" << (index + 1) << " / " << m_bm.size()
            << ": " << bm.name;
   }
   else
   {
      out() << "  --\"--";
   }

   if (bm.threads[variant] > 1)
      out() << " ×" << bm.threads[variant] << " threads";

   out() << std::endl;
}

void Runner::printResult(
      std::size_t index,
      std::size_t variant
   )
{
   auto best = ns(
      m_bm.front().data.front().wallTime
   );

   auto& bm = m_bm[index];

   if (variant == 0)
   {
      m_console.line(out(), '-');

      out() << "   " << bm.name << std::endl;
      out() << "   ";
      m_console.line(
         out(),
         '-',
         std::min(
            unsigned(bm.name.length()),
            unsigned(m_console.width() - 6)
         )
      );
   }

   auto wall = ns(bm.data[variant].wallTime);
   auto cpu = ns(bm.data[variant].cpuTime);
   auto op = cpu / double(m_iterations * bm.threads[variant]);
   auto percent = wall * 100.0 / double(best);

   out() << std::setw(2) << bm.threads[variant] << " |"
         << std::setw(12) << (wall / 1000) <<  " |";

   if  (op < 1)
   {
      out() << std::setw(7)
            << std::setprecision(2) << std::fixed
            << op << " | ";
   }
   else
   {
      out() << std::setw(7)
            << std::round(op) << " | ";
   }

   if (percent < 1)
   {
      out() << std::setw(5)
            << std::setprecision(2) << std::fixed
            << percent;
   }
   else
   {
      out() << std::setw(5) << std::round(percent);
   }

   auto u = ms(bm.data[variant].cpuUsage.user);
   out() << " | " << u;

   auto s = ms(bm.data[variant].cpuUsage.system);
   if (s > 0)
      out() << " / " << s;

   out() << std::endl;
}

void Runner::printHeader()
{
   m_console.line(out(), '-');

   out() << " × |"
         << "  Total, µs  |"
         << " Op, ns |"
         << "   %   |"
         << " CPU (u/s), ms"
         << std::endl;
}

void Runner::run()
{
   printCaption();

   if (m_bm.empty())
      return;

   for (std::size_t index = 0; index < m_bm.size(); ++index)
   {
      auto& bm =  m_bm[index];
      bm.data.resize(bm.threads.size());

      for (std::size_t variant = 0; variant < bm.threads.size(); ++variant)
      {
         printRunning(index, variant);

         bm.data[variant] = ::Benchmark::run(
            bm.threads[variant],
            bm.work.get(),
            m_iterations
         );
      }
   }

   printHeader();

   std::size_t id = 0;
   for (std::size_t index = 0; index < m_bm.size(); ++index)
   {
      auto& bm =  m_bm[index];
      for (std::size_t variant = 0; variant < bm.threads.size(); ++variant)
      {
         printResult(index, variant);
      }
   }

   printFooter();
}


} // namespace
