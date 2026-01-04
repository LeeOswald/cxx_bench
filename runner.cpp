#include "runner.hpp"


namespace Benchmark
{

void Runner::run()
{
   m_console.line('=');

   out() << m_name;
   if (m_iterations > 0)
      out() << " (" << m_iterations << " iterations)";

   out() <<  std::endl;

   m_console.line('-');

   if (m_bm.empty())
      return;

   std::string name;
   std::size_t index = 0;

   for (auto& bm: m_bm)
   {
      if (bm.group)
         name = bm.name;

      out() << "#" << (index + 1) << " / " << m_bm.size()
            << ": " << name;

      if (bm.threads > 1)
         out() << " ×" << bm.threads << " threads";

      out() << std::endl;

      bm.timings = ::Benchmark::run(
         bm.threads,
         bm.work,
         m_iterations
      );

      ++index;
   }

   m_console.line('-');

   out() << " × |"
         << " Total, µs |"
         << " Op, ns |"
         << "    %    |"
         << " CPU (u/s), ms"
         << std::endl;

   auto best = m_bm.front().timings.time.count();

   std::size_t id = 0;
   for (auto& bm: m_bm)
   {
      if (id == 0 || bm.group)
         m_console.line('-');

      if (bm.group)
      {
         out() << " " << bm.name << std::endl;
         out() << "   ";
         m_console.line('-', m_console.width() - 3);
      }

      auto du = bm.timings.time.count();
      auto op = du / m_iterations;
      auto percent = du * 100.0 / double(best);

      out() << std::setw(2) << bm.threads << " |"
            << std::setw(10) << (du / 1000) <<  " |"
            << std::setw(7) << op << " | ";

      if (percent < 200)
      {
         out() << std::setw(7) << std::setprecision(2) << std::fixed
               << percent;
      }
      else
      {
         out() << std::setw(7)
               << unsigned(percent);
      }

      {
         auto u = bm.timings.cpu.user.count() / 1000;
         out() << " | " << u;

         auto s = bm.timings.cpu.system.count() / 1000;
         if (s > 0)
            out() << " / " << s;
      }

      out() << std::endl;
      ++id;
   }

   m_console.line('-');
}


} // namespace
