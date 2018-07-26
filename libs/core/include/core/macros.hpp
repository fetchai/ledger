#ifndef FETCH_MACROS_HPP
#define FETCH_MACROS_HPP

#define FETCH_UNUSED(x)   (void)(x)

#ifdef __GNUC__
  #define FETCH_MAYBE_UNUSED __attribute__((used))
#else
  #define FETCH_MAYBE_UNUSED
#endif

#endif //FETCH_MACROS_HPP
