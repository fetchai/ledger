#include "vectorise/math/exp.hpp"
#include "vectorise/memory/array.hpp"
#include "vectorise/memory/shared_array.hpp"
#include <cmath>
#include <iostream>

typedef double                                    type;
typedef fetch::memory::Array<type>                array_type;
typedef typename array_type::vector_register_type vector_type;

void Exponentials(array_type const &A, array_type &C)
{
  C.in_parallel().Apply([](vector_type const &a, vector_type &c) { c = fetch::vectorize::exp(a); },
                        A);
}

int main(int argc, char const **argv)
{
  if (argc != 2)
  {
    std::cout << std::endl;
    std::cout << "Usage: " << argv[0] << " [array size] " << std::endl;
    std::cout << std::endl;
    return 0;
  }

  std::size_t N = std::size_t(atoi(argv[1]));
  array_type  A(N), C(N);

  for (std::size_t i = 0; i < N; ++i)
  {
    A[i] = double(0.1 * type(i) - type(double(N) * 0.5));
  }

  Exponentials(A, C);
  for (std::size_t i = 0; i < N; ++i)
  {
    std::cout << A[i] << " " << C[i] << " " << std::exp(A[i]) << std::endl;
  }

  return 0;
}
