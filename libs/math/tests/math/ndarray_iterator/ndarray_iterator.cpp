#include <iomanip>
#include <iostream>

//#include "core/random/lcg.hpp"
#include <gtest/gtest.h>

//#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"
#include "math/ndarray_iterator.hpp"
#include "math/ndarray_view.hpp"
//#include "vectorise/threading/pool.hpp"

using namespace fetch::math;


TEST(ndarray, simple_iterator_permute_test)
{

  // set up an initial array
  NDArray<double> array{NDArray<double>::Arange(0, 77, 1)};
  array.Reshape({7, 11});

  NDArray<double> ret;
  ret.ResizeFromShape(array.shape());
//  NDArray< double > ret = NDArray< double >::Zeros(25);
//  ret.Reshape({5, 5});
  ASSERT_TRUE( ret.size() == array.size() );
  ASSERT_TRUE( ret.shape() == array.shape() );  
  NDArrayIterator<double, NDArray<double>::container_type> it(array);
  NDArrayIterator<double, NDArray<double>::container_type> it2(ret);

  it2.PermuteAxes(0,1);
  while(it2)
  {
    ASSERT_TRUE( bool(it) );
    ASSERT_TRUE( bool(it2));

    *it2 = *it;
    ++it;
    ++it2;
  }

  std::size_t test_val, cur_row;
  for (std::size_t i=0; i < array.size(); ++i)
  {
    ASSERT_TRUE(array[i] == i);

    cur_row = i / 7;
    test_val = (11 * (i%7)) + cur_row;

    ASSERT_TRUE(ret[i] == test_val);
  }
}

TEST(ndarray, iterator_4dim_permute_test)
{

  // set up an initial array
  NDArray<double> array{NDArray<double>::Arange(0, 1008, 1)};
  array.Reshape({4, 6, 7, 6});
  NDArray<double> ret = array.Copy();

  NDArrayIterator<double, NDArray<double>::container_type> it(array, {{1, 2, 1}, {2, 3, 1}, {1, 4, 1}, {2, 6, 1}});
  NDArrayIterator<double, NDArray<double>::container_type> it2(ret, {{1, 2, 1}, {2, 3, 1}, {1, 4, 1}, {2, 6, 1}});

  it2.PermuteAxes(0,1);
  it2.PermuteAxes(2,3);
  while(it2)
  {

    assert( bool(it) );
    assert( bool(it2));

    *it2 = *it;
    ++it;
    ++it2;
  }
  

  for (std::size_t i=0; i < array.size(); ++i)
  {
    for (std::size_t j=0; j < array.size(); ++j)
    {
      for (std::size_t k=0; k < array.size(); ++k)
      {
        for (std::size_t l=0; l < array.size(); ++l)
        {
          ASSERT_TRUE(ret.Get({i,j,k,l}) == array.Get({j,i,l,k}));
        }
      }
    }
  }

}