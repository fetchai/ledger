#include "vectorise/memory/shared_array.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>

using namespace fetch::memory;

typedef float                                       type;
typedef SharedArray<type>                           ndarray_type;
typedef typename ndarray_type::vector_register_type vector_register_type;
#define M 10000   // 100000
#define N 100000  // 4 * 100000

int main(int argc, char **argv)
{
  ndarray_type a(N), c(N), b(N);

  for (std::size_t i = 0; i < N; ++i)
  {
    b[i] = type(i);
  }

  if ((argc >= 2) && (std::string(argv[1]) == "comp"))
  {

    // Standard implementation
    for (std::size_t i = 0; i < M; ++i)
    {
      for (std::size_t j = 0; j < N; j += 4)
      {

        // We write it out such that the compiler might use SSE
        a[j]     = std::exp(1 + std::log(b[j]));
        a[j + 1] = std::exp(1 + std::log(b[j + 1]));
        a[j + 2] = std::exp(1 + std::log(b[j + 2]));
        a[j + 3] = std::exp(1 + std::log(b[j + 3]));
      }
    }
  }
  else if ((argc >= 2) && (std::string(argv[1]) == "kernel"))
  {
    for (std::size_t i = 0; i < M; ++i)
    {

      // Here we use a kernel to compute the same, using an approximation
      a.in_parallel().Apply(
          [](vector_register_type const &x, vector_register_type &y) {
            static vector_register_type one(1);

            // We approximate the exponential function by a clever first order
            // Taylor expansion
            y = approx_exp(one + approx_log(x));
          },
          b);
    }
  }

  return 0;
}
