#include "math/ndarray_broadcast.hpp"
#include "math/ndarray.hpp"
#include "math/ndarray_iterator.hpp"
#include <iomanip>
#include <iostream>

using namespace fetch::math;

int main()
{
  NDArray<double> a = NDArray<double>::Arange(0, 20, 1);
  a.Reshape({1, a.size()});

  NDArray<double> b = NDArray<double>::Arange(0, 20, 1);
  b.Reshape({b.size(), 1});

  NDArray<double> c = NDArray<double>::Zeros(a.size() * b.size());

  if (!Broadcast([](double const &x, double const &y) { return x + y; }, a, b, c))
  {
    std::cout << "Broadcast failed!" << std::endl;
    exit(-1);
  }
  std::cout << "Done broadcasting" << std::endl;

  for (std::size_t i = 0; i < c.shape(0); ++i)
  {
    for (std::size_t j = 0; j < c.shape(1); ++j)
    {
      std::cout << std::setw(3) << c({i, j});
    }
    std::cout << std::endl;
  }

  std::cout << std::endl;

  return 0;
}
