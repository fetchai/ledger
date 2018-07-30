// #include"math/ndarray.hpp"
#include "math/exp.hpp"
#include "math/kernels/concurrent_vm.hpp"
#include "math/linalg/matrix.hpp"
#include "math/rectangular_array.hpp"
#include "vectorise/threading/pool.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
using namespace fetch::memory;
using namespace fetch::threading;
using namespace std::chrono;

using type                 = double;
using ndarray_type         = SharedArray<type>;
using vector_register_type = typename ndarray_type::vector_register_type;
#define N 200

int main(int argc, char **argv)
{

  ndarray_type a(N), b(N), c(N);

  type ref = 0;
  Pool pool(4);

  std::size_t SS = 3, BB = 7;

  for (std::size_t i = 0; i < N; ++i)
  {
    a[i] = type(i);
    b[i] = 2 * type(i);
    c[i] = type(i & 1) + 1;
    if ((SS <= i) && (i <= BB))
    {
      ref += (a[i] + b[i]) * c[i];
    }
  }

  std::cout << vector_register_type::dsp_sum_of_products(a.pointer(), b.pointer(), N) << std::endl;

  return 0;

  fetch::kernels::ConcurrentVM<vector_register_type> vm;
  vm.AddInstruction(1, 0, 1, 2);
  vm.AddInstruction(2, 0, 1, 1);
  vm.AddInstruction(4, 2, 1, 1);

  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  c.in_parallel().Apply(vm, a, b);
  high_resolution_clock::time_point t2  = high_resolution_clock::now();
  duration<double>                  ts1 = duration_cast<duration<double>>(t2 - t1);

  high_resolution_clock::time_point t3 = high_resolution_clock::now();
  for (std::size_t i = 0; i < N; ++i)
  {
    c[i] = (a[i] + b[i]) / (a[i] - b[i]);
  }
  high_resolution_clock::time_point t4  = high_resolution_clock::now();
  duration<double>                  ts2 = duration_cast<duration<double>>(t4 - t3);

  fetch::kernels::ConcurrentVM<type> vm2;
  vm2.AddInstruction(1, 0, 1, 2);
  vm2.AddInstruction(2, 0, 1, 1);
  vm2.AddInstruction(4, 2, 1, 1);

  high_resolution_clock::time_point t5 = high_resolution_clock::now();
  c.in_parallel().Apply(vm2, a, b);
  high_resolution_clock::time_point t6  = high_resolution_clock::now();
  duration<double>                  ts3 = duration_cast<duration<double>>(t6 - t5);

  high_resolution_clock::time_point t7 = high_resolution_clock::now();
  c.in_parallel().Apply([](vector_register_type const &x, vector_register_type const &y,
                           vector_register_type &z) { z = x + y; },
                        a, b);

  c.in_parallel().Apply([](vector_register_type const &x, vector_register_type const &y,
                           vector_register_type &z) { z = z / (x - y); },
                        a, b);
  high_resolution_clock::time_point t8  = high_resolution_clock::now();
  duration<double>                  ts4 = duration_cast<duration<double>>(t8 - t7);

  std::cout << "Non-concurrent VM: " << ts3.count() * 1000 << " ms" << std::endl;
  std::cout << "Concurrent VM: " << ts1.count() * 1000 << " ms" << std::endl;
  std::cout << "Vectorised ops: " << ts4.count() * 1000 << " ms" << std::endl;
  std::cout << "Native C++: " << ts2.count() * 1000 << " ms" << std::endl;

  /*
  for(std::size_t i=0; i < N; ++i) {
    std::cout << c[i] << " " ;
  }
  std::cout << std::endl;
  */
  return 0;

  std::cout << "Did this break? "
            << a.in_parallel().SumReduce(
                   TrivialRange(SS, BB + 1),
                   [](vector_register_type const &x, vector_register_type const &y,
                      vector_register_type const &z) { return (x + y) * z; },
                   b, c)
            << std::endl;
  std::cout << "Compare with: " << ref << std::endl;

  /*
  b.in_parallel().Apply(TrivialRange(100, 599), [](vector_register_type const
  &x, vector_register_type &y) { y = x;
    }, a);

  for(std::size_t i=0; i < N; ++i) {
    std::cout << b[i] << " ";
  }

  std::cout << std::endl;
  */
  /*
    c.Equal( a, b );
    for(std::size_t i=0; i < N; ++i) {
      std::cout << a[i] << " == " << b[i] << " => " << c[i] << std::endl;
    }
  */
  /*
    RectangularArray< type > d;
    a.Reshape( {10, 10, 20} );
    std::cout << a(1,2,3) << " " <<  (3 + 20 * 2 + 1*10*20) << std::endl;
    std::cout << a(1,2) << std::endl;

    fetch::math::linalg::Matrix< type > m, vec;

    a.Get(d, 9);
    std::cout << d.height() << " " << d.width() << std::endl;

    a.Get(m, 1);
    std::cout << m.height() << " " << m.width() << std::endl;

    for(std::size_t i = 0; i < a.shape()[0]; ++i) {
      for(std::size_t j = 0; j < a.shape()[1]; ++j) {

        a.Get(vec, i, j);
        for(std::size_t k=0 ; k < vec.size(); ++k ) {
          std::cout << vec[k] << " ";
        }

        std::cout << std::endl;
      }
    }

  */

  return 0;
}
