#include <iomanip>
#include <iostream>

//#include "core/random/lcg.hpp"
#include <gtest/gtest.h>

#include "math/exp.hpp"
#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"
#include "math/ndarray_view.hpp"
#include "vectorise/threading/pool.hpp"

// using namespace fetch::memory;
// using namespace fetch::threading;
// using namespace std::chrono;

using namespace fetch::math;

using data_type            = double;
using container_type       = fetch::memory::SharedArray<data_type>;
using vector_register_type = typename container_type::vector_register_type;
#define N 200

template <typename D>
using _S = fetch::memory::SharedArray<D>;

template <typename D>
using _A = NDArray<D, _S<D>>;

TEST(ndarray, 2d_view_full)
{
  // set up a 4d array
  std::vector<std::size_t> shape = {5, 5};
  _A<double> test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (5*5); ++i)
  {
    test_array[i] = i;
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0,5,1},{0,5,1}};
  NDArrayView array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {5, 5};
  _A<double> new_array = _A<double>(new_shape);
  new_array = test_array.GetRange(array_view);

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }

}

TEST(ndarray, 3d_view_full)
{
  // set up a 4d array
  std::vector<std::size_t> shape = {5, 5, 5};
  _A<double> test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (5*5*5); ++i)
  {
    test_array[i] = i;
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0,5,1},{0,5,1},{0,5,1}};
  NDArrayView array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {5, 5, 5};
  _A<double> new_array = _A<double>(new_shape);
  new_array = test_array.GetRange(array_view);

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }

}

TEST(ndarray, 4d_view_full)
{
  // set up a 4d array
  std::vector<std::size_t> shape = {5, 5, 5, 5};
  _A<double> test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (5*5*5*5); ++i)
  {
    test_array[i] = i;
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0,5,1},{0,5,1},{0,5,1},{0,5,1}};
  NDArrayView array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {5, 5, 5, 5};
  _A<double> new_array = _A<double>(new_shape);
  new_array = test_array.GetRange(array_view);

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }
}

TEST(ndarray, 6d_view_full)
{
  // set up a 4d array
  std::vector<std::size_t> shape = {5, 5, 5, 5, 5, 5};
  _A<double> test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (5*5*5*5*5*5); ++i)
  {
    test_array[i] = i;
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0,5,1},{0,5,1},{0,5,1},{0,5,1},{0,5,1},{0,5,1}};
  NDArrayView array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {5, 5, 5, 5, 5, 5};
  _A<double> new_array = _A<double>(new_shape);
  new_array = test_array.GetRange(array_view);

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }

}

TEST(ndarray, 2d_irregular_view)
{
  // set up a 4d array
  std::vector<std::size_t> shape = {5, 10};
  _A<double> test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (5 * 10); ++i)
  {
    test_array[i] = i;
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0, 5, 1},{0, 10, 1}};
  NDArrayView array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  std::cout << "array view set up" << std::endl;

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {5, 10};
  _A<double> new_array = _A<double>(new_shape);
  new_array = test_array.GetRange(array_view);

  std::cout << "getrange complete " << std::endl;

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }

}


TEST(ndarray, 3d_irregular_view)
{
  // set up a 4d array
  std::vector<std::size_t> shape = {5, 10, 10};
  _A<double> test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (5 * 10 * 10); ++i)
  {
    test_array[i] = i;
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0, 5, 1}, {0, 10, 1}, {0, 10, 1}};
  NDArrayView array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  std::cout << "array view set up" << std::endl;

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {5, 10, 10};
  _A<double> new_array = _A<double>(new_shape);
  new_array = test_array.GetRange(array_view);

  std::cout << "getrange complete " << std::endl;

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }

}

TEST(ndarray, 6d_irregular_view)
{
  // set up a 4d array
  std::vector<std::size_t> shape = {1, 2, 3, 4, 5, 6};
  _A<double> test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (1 * 2 * 3 * 4 * 5 * 6); ++i)
  {
    test_array[i] = i;
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0,1,1},{0,2,1},{0,3,1},{0,4,1},{0,5,1},{0,6,1}};
  NDArrayView array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  std::cout << "array view set up" << std::endl;

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {0, 1, 2, 3, 4, 5, 6};
  _A<double> new_array = _A<double>(new_shape);
  new_array = test_array.GetRange(array_view);

  std::cout << "getrange complete " << std::endl;

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    // std::cout << test_array[i] << std::endl;
    // std::cout << new_array[i] << std::endl;
    // std::cout << std::endl;
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }
}



TEST(ndarray, 2d_big_step)
{
  // set up a 4d array
  std::vector<std::size_t> shape = {8, 8};
  _A<double> test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (8 * 8); ++i)
  {
    test_array[i] = i;
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0, 8, 4},{0, 8, 4}};
  NDArrayView array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  std::cout << "array view set up" << std::endl;

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {8, 8};
  _A<double> new_array = _A<double>(new_shape);
  new_array = test_array.GetRange(array_view);

  std::cout << "getrange complete " << std::endl;

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    // std::cout << test_array[i] << std::endl;
    // std::cout << new_array[i] << std::endl;
    // std::cout << std::endl;
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }
}




//
//
//TEST(ndarray, regular_subset_view)
//{
//  // set up a 4d array
//  std::vector<std::size_t> shape = {6, 6, 6};
//  _A<double> test_array = _A<double>(shape);
//  for (std::size_t i = 0; i < (6*6*6); ++i)
//  {
//    test_array[i] = i;
//  }
//
//  // set up a valid view shape
//  std::vector<std::vector<std::size_t>> view_shape = {{0,3,1},{0,3,1},{0,3,1}};
//  NDArrayView array_view = NDArrayView();
//  for (auto item : view_shape)
//  {
//    array_view.from.push_back(item[0]);
//    array_view.to.push_back(item[1]);
//    array_view.step.push_back(item[2]);
//  }
//
//  std::cout << "array view set up" << std::endl;
//
//  // extract the view shape into a new ndarray
//  std::vector<std::size_t> new_shape = {3, 3, 3};
//  _A<double> new_array = _A<double>(new_shape);
//  new_array = test_array.GetRange(array_view);
//
//  std::cout << "getrange complete " << std::endl;
//
//  for (std::size_t i = 0; i < new_array.data().size(); ++i)
//  {
//    // std::cout << test_array[i] << std::endl;
//    // std::cout << new_array[i] << std::endl;
//    // std::cout << std::endl;
//    ASSERT_TRUE(test_array[i] == new_array[i]);
//  }
//}
//
//
//
//


