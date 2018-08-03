// #include"math/ndarray.hpp"
#include "math/exp.hpp"
#include "math/linalg/matrix.hpp"
#include "math/rectangular_array.hpp"
#include "vectorise/threading/pool.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
using namespace fetch::memory;
using namespace fetch::threading;
using namespace std::chrono;

typedef double                                      type;
typedef SharedArray<type>                           ndarray_type;
typedef typename ndarray_type::vector_register_type vector_register_type;
#define N 200

int main(int argc, char **argv)
{

  ndarray_type a(N), b(N), c(N);

  return 0;
}
