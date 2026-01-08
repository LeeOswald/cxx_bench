#pragma once

#include <cstdio>
#include <iostream>
#include <locale>


#if BM_POSIX
   #include <sys/ioctl.h>
   #include <unistd.h>
#endif

#if BM_WINDOWS
   #include <io.h>
   #include <windows.h>
#endif


namespace Benchmark
{

class Terminal
{
public:
   Terminal() noexcept
      : m_redirected(isRedirected())
      , m_locale(std::locale(), new numpunct)
   {
      detectWindowSize();

      std::cout.imbue(m_locale);
      std::cerr.imbue(m_locale);
   }

   bool redirected() const noexcept
   {
      return m_redirected;
   }

   auto& out() noexcept
   {
      return std::cout;
   }

   auto& err() noexcept
   {
      return std::cerr;
   }

   auto width() const noexcept
   {
      return m_width;
   }

   auto height() const noexcept
   {
      return m_height;
   }

   void line(
      auto& stream,
      char c,
      int width = -1,
      bool eol = true
   )
   {
      if (width < 0)
         width = this->width();

      while (width--)
         stream << c;

      if (eol)
         stream << std::endl;
   }

private:
   class numpunct
      : public std::numpunct<char>
   {
   protected:
      std::string do_grouping() const override
      {
         return "\003";
      }

      char do_thousands_sep() const override
      {
         return ',';
      }
   };

   static bool isRedirected() noexcept
   {
#if BM_POSIX
      return ::isatty(::fileno(::stdout)) == 0;
#elif BM_WINDOWS
      return ::_isatty(::_fileno(::stdout)) == 0;
#else
      return false;
#endif
   }

   void detectWindowSize() noexcept
   {
      if (redirected())
         return;
#if BM_WINDOWS
      CONSOLE_SCREEN_BUFFER_INFO csbi = {};
      ::GetConsoleScreenBufferInfo(
         ::GetStdHandle(STD_OUTPUT_HANDLE), 
         &csbi
      );

      m_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
      m_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#elif BM_POSIX 
      struct winsize size;
      ::ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
      m_width = size.ws_col;
      m_height = size.ws_row;
#endif
   }

   bool m_redirected;
   std::locale m_locale;
   int m_width = 80;
   int m_height = 25;
};


} // namespace

