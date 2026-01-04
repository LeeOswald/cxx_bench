#pragma once

#include <cstdio>
#include <iostream>


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
   {
      detectWindowSize();
   }

   bool redirected() const noexcept
   {
      return m_redirected;
   }

   static auto& out() noexcept
   {
      return std::cout;
   }

   auto width() const noexcept
   {
      return m_width;
   }

   auto height() const noexcept
   {
      return m_height;
   }

   void line(char c, int width = -1, bool eol = true)
   {
      if (width < 0)
         width = this->width();

      while (width--)
         out() << c;

      if (eol)
         out() << std::endl;
   }

private:
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
   int m_width = 80;
   int m_height = 25;
};


} // namespace

