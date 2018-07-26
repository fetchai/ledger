#pragma once

#define FETCH_UNUSED(x)   (void)(x)

#ifdef __GNUC__
  #define FETCH_MAYBE_UNUSED __attribute__((used))
#else
  #define FETCH_MAYBE_UNUSED
#endif

