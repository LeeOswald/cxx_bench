#include "chrono.hpp"
#include "runner.hpp"

#include <iomanip>


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

   if (m_items.empty())
      return;

   std::string name;
   std::size_t index = 0;

   for (auto& item: m_items)
   {
      if (item.group)
         name = item.name;

      out() << "#" << (index + 1) << " / " << m_items.size()
            << ": " << name;

      if (item.threads > 1)
         out() << " ×" << item.threads << " threads";

      out() << std::endl;

      item.data = ::Benchmark::run(
         item.threads,
         item.work.get(),
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

   auto best = ns(m_items.front().data.wallTime);

   std::size_t id = 0;
   for (auto& item: m_items)
   {
      if (id == 0 || item.group)
         m_console.line('-');

      if (item.group)
      {
         out() << " " << item.name << std::endl;
         out() << "   ";
         m_console.line('-', m_console.width() - 3);
      }

      auto wall = ns(item.data.wallTime);
      auto cpu = ns(item.data.cpuTime);
      auto op = cpu / m_iterations;
      auto percent = wall * 100.0 / double(best);

      out() << std::setw(2) << item.threads << " |"
            << std::setw(10) << (wall / 1000) <<  " |"
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
         auto u = ms(item.data.cpuUsage.user);
         out() << " | " << u;

         auto s = ms(item.data.cpuUsage.system);
         if (s > 0)
            out() << " / " << s;
      }

      out() << std::endl;
      ++id;
   }

   m_console.line('-');
}


} // namespace
