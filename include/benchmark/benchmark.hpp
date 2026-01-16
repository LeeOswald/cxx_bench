#pragma once


#if __linux__
   #define BM_POSIX 1
#endif


#define BM_NOINLINE \
   __attribute__((noinline))

#define BM_DONT_OPTIMIZE \
  __attribute__((optnone))
