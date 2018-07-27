#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wmany-braces-around-scalar-init"
#pragma GCC diagnostic ignored "-Wbraced-scalar-init"
#pragma GCC diagnostic ignored "-Wc++11-compat-pedantic"
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wpedantic"
#pragma clang diagnostic ignored "-pedantic"
#pragma clang diagnostic ignored "-Wall"
#pragma clang diagnostic ignored "-Wextra"
#pragma clang diagnostic ignored "-Wmany-braces-around-scalar-init"
#pragma clang diagnostic ignored "-Wc++11-compat-pedantic"
#pragma clang diagnostic ignored "-Wbraced-scalar-init"
#pragma clang diagnostic ignored "-Weverything"
#endif

#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
