#pragma once

#define BM_NOINLINE \
   __attribute__((noinline))

#define BM_DONT_OPTIMIZE \
  __attribute__((optnone))
