#include "math/ndarray_iterator.hpp"
#include "math/ndarray.hpp"
#include "math/ndarray_broadcast.hpp"
#include <iostream>

using namespace fetch::math;

int main()
{
  NDArray<double> array = NDArray<double>::Arange(0, 100, 1);
  array.Reshape({5, 20});
  for (std::size_t i = 0; i < array.shape(0); ++i)
  {
    std::size_t k = i;
    for (std::size_t j = 0; j < array.shape(1); ++j)
    {
      std::cout << array[k] << " ";
      k += array.shape(0);
    }
    std::cout << std::endl;
  }

  NDArray<double> ret = NDArray<double>::Zeros(100);
  ret.Reshape({5, 20});

  std::cout << "XXX: " << array.size() << " " << ret.size() << std::endl;
  NDArrayIterator<double, NDArray<double>::container_type> it(array, {{1, 4}, {3, 19, 3}});
  NDArrayIterator<double, NDArray<double>::container_type> it2(ret, {{1, 4}, {3, 19, 3}});

  it2.PermuteAxes(0, 1);
  while (it2)
  {

    assert(bool(it));
    assert(bool(it2));

    *it2 = *it;
    ++it;
    ++it2;
  }

  std::cout << std::endl;

  for (std::size_t i = 0; i < ret.shape(0); ++i)
  {
    std::size_t k = i;
    for (std::size_t j = 0; j < ret.shape(1); ++j)
    {
      std::cout << ret[k] << " ";
      k += ret.shape(0);
    }
    std::cout << std::endl;
  }

  NDArrayIterator<double, NDArray<double>::container_type> it3(array, {{1, 2}, {1, 4}});
  if (!UpgradeIteratorFromBroadcast<double, NDArray<double>::container_type>({2, 2, 4, 3}, it3))
  {
    std::cout << "Failed in broad casting" << std::endl;
    return 0;
  }
  std::cout << "Broadcast: ";
  while (it3)
  {
    std::cout << (*it3) << " ";
    ++it3;
  }
  std::cout << std::endl;

  return 0;
}
