//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

// #include <algorithm>
#include <iomanip>
#include <iostream>
#include <gtest/gtest.h>

#include "math/linalg/matrix.hpp"
#include "ml/ops/ops.hpp"

//#include "math/ndarray.hpp"
//#include "math/free_functions/free_functions.hpp"

using namespace fetch::ml;
#define ARRAY_SIZE 100

using Type = double;
using ArrayType = fetch::math::linalg::Matrix<Type>;


TEST(loss_functions, Dot_test)
{

  using ArrayType = fetch::math::linalg::Matrix<Type>;
  using LayerType = fetch::ml::Variable<ArrayType>;

  SessionManager<ArrayType, LayerType> sess{};

  std::vector<std::size_t> l1_shape{2, 3};
  std::vector<std::size_t> l2_shape{3, 4};

  Variable<ArrayType> l1{sess, l1_shape};
  Variable<ArrayType> l2{sess, l2_shape};

  std::size_t counter = 0;


  Type setval = 0.;
  for (std::size_t i = 0; i < l1_shape[0]; ++i)
  {
    for (std::size_t j = 0; j < l1_shape[1]; ++j)
    {
      l1.Set(i, j, setval);
      setval += 1.0;
    }
  }

  setval = 0.;
  for (std::size_t i = 0; i < l2_shape[0]; ++i)
  {
    for (std::size_t j = 0; j < l2_shape[1]; ++j)
    {
      l2.Set(i, j, setval);
      setval += 1.0;
    }
  }

  Variable<ArrayType> ret = fetch::ml::ops::Dot(l1, l2, sess);
  ASSERT_TRUE(ret.shape()[0] == l1_shape[0]);
  ASSERT_TRUE(ret.shape()[1] == l2_shape[1]);

  std::vector<Type> gt{20, 23, 26, 29, 56, 68, 80, 92};
  for (std::size_t i = 0; i < ret.shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < ret.shape()[1]; ++j)
    {
      ASSERT_TRUE(ret.At(i, j) == gt[counter]);
      ++counter;
    }
  }
}

TEST(loss_functions, Relu_test)
{

  using ArrayType = fetch::math::linalg::Matrix<Type>;
  using LayerType = fetch::ml::Variable<ArrayType>;
  SessionManager<ArrayType, LayerType> sess{};
  std::size_t counter = 0;

  std::vector<std::size_t> l1_shape{2, 3};

  Variable<ArrayType> l1{sess, l1_shape};

  Type setval = -3.;
  for (std::size_t i = 0; i < l1_shape[0]; ++i)
  {
    for (std::size_t j = 0; j < l1_shape[1]; ++j)
    {
      l1.Set(i, j, setval);
      setval += 1.0;
    }
  }

  Variable<ArrayType> ret = fetch::ml::ops::Relu(l1, sess);

  ASSERT_TRUE(ret.shape() == l1.shape());

  std::vector<Type> gt{0, 0, 0, 0, 1, 2};
  counter = 0;
  for (std::size_t i = 0; i < ret.shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < ret.shape()[1]; ++j)
    {
      ASSERT_TRUE(ret.At(i, j) == gt[counter]);
      ++counter;
    }
  }
}

TEST(loss_functions, Sigmoid_test)
{

  using ArrayType = fetch::math::linalg::Matrix<Type>;
  using LayerType = fetch::ml::Variable<ArrayType>;
  SessionManager<ArrayType, LayerType> sess{};
  std::vector<std::size_t> l1_shape{2, 3};

  Variable<ArrayType> l1{sess, l1_shape};

  Type setval = -3.;
  for (std::size_t i = 0; i < l1_shape[0]; ++i)
  {
    for (std::size_t j = 0; j < l1_shape[1]; ++j)
    {
      l1.Set(i, j, setval);
      setval += 1.0;
    }
  }

  Variable<ArrayType> ret = fetch::ml::ops::Sigmoid(l1, sess);

  ASSERT_TRUE(ret.shape() == l1.shape());

  ArrayType gt{l1.shape()};
  gt.Set(0, 0.95257412682243336);
  gt.Set(1, 0.5);
  gt.Set(2, 0.88079707797788231);
  gt.Set(3, 0.2689414213699951);
  gt.Set(4, 0.7310585786300049);
  gt.Set(5, 0.11920292202211755);

  ASSERT_TRUE(ret.data().AllClose(gt));

}

TEST(loss_functions, Sum_test)
{

  using ArrayType = fetch::math::linalg::Matrix<Type>;
  using LayerType = fetch::ml::Variable<ArrayType>;
  SessionManager<ArrayType, LayerType> sess{};
  std::size_t counter = 0;

  std::vector<std::size_t> l1_shape{2, 3};

  Variable<ArrayType> l1{sess, l1_shape};

  Type setval = 0.;
  for (std::size_t i = 0; i < l1_shape[0]; ++i)
  {
    for (std::size_t j = 0; j < l1_shape[1]; ++j)
    {
      l1.Set(i, j, setval);
      ++setval;
    }
  }

  Variable<ArrayType> ret = fetch::ml::ops::Sum(l1, 1, sess);

  ASSERT_TRUE(ret.shape()[0] == l1.shape()[0]);
  ASSERT_TRUE(ret.shape()[1] == 1);

  std::vector<Type> gt{3, 12};
  counter = 0;
  for (std::size_t i = 0; i < ret.shape()[0]; ++i)
  {
    ASSERT_TRUE(ret.At(i, 0) == gt[counter]);
    ++counter;
  }
}


TEST(loss_functions, MSE_test)
{

  using ArrayType = fetch::math::linalg::Matrix<Type>;
  using LayerType = fetch::ml::Variable<ArrayType>;
  SessionManager<ArrayType, LayerType> sess{};

  std::vector<std::size_t> shape{2, 3};

  Variable<ArrayType> l1{sess, shape};
  Variable<ArrayType> l2{sess, shape};

  Type setval = 0.;
  for (std::size_t i = 0; i < shape[0]; ++i)
  {
    for (std::size_t j = 0; j < shape[1]; ++j)
    {
      l1.Set(i, j, setval);
      ++setval;
      l2.Set(i, j, setval);
    }
  }

  Variable<ArrayType> ret = fetch::ml::ops::MeanSquareError(l1, l2, sess);

  ASSERT_TRUE(ret.shape()[0] == 1);
  ASSERT_TRUE(ret.shape()[1] == 1);

  std::vector<Type> gt{6};
  ASSERT_TRUE(ret.At(0, 0) == gt[0]);
}


TEST(loss_functions, CEL_test)
{

  using ArrayType = fetch::math::linalg::Matrix<Type>;
  using LayerType = fetch::ml::Variable<ArrayType>;
  SessionManager<ArrayType, LayerType> sess{};

  std::vector<std::size_t> shape{3, 3};

  Variable<ArrayType> l1{sess, shape};
  Variable<ArrayType> l2{sess, shape};

  l1.Set(0, 0, 0.1);   l1.Set(0, 1, 0.8);   l1.Set(0, 2, 0.1);
  l1.Set(1, 0, 0.8);   l1.Set(1, 1, 0.1);   l1.Set(1, 2, 0.1);
  l1.Set(2, 0, 0.1);   l1.Set(2, 1, 0.1);   l1.Set(2, 2, 0.8);
  
  l2.Set(0, 0, 1);   l2.Set(0, 1, 0);   l2.Set(0, 2, 0);
  l2.Set(1, 0, 1);   l2.Set(1, 1, 0);   l2.Set(1, 2, 0);
  l2.Set(2, 0, 0);   l2.Set(2, 1, 0);   l2.Set(2, 2, 1);

  Variable<ArrayType> ret = fetch::ml::ops::CrossEntropyLoss(l1, l2, sess);

  ASSERT_TRUE(ret.shape()[0] == 1);
  ASSERT_TRUE(ret.shape()[1] == 1);

  ASSERT_TRUE(ret.At(0, 0) == 2.7488721956224649);
}

//TEST(loss_functions, MSE_Test)
//{
//
//  fetch::math::ShapeLessArray<T> y_arr = fetch::math::ShapeLessArray<T>::UniformRandom(ARRAY_SIZE);
//  fetch::math::ShapeLessArray<T> y_hat_arr =
//      fetch::math::ShapeLessArray<T>::UniformRandom(ARRAY_SIZE);
//
//  T result      = -999;
//  T test_result = -999;
//
//  fetch::math::ShapeLessArray<T> test_result_arr = fetch::math::ShapeLessArray<T>(y_arr.size());
//  // error
//  Subtract(y_arr, y_hat_arr, test_result_arr);
//  // square
//  Square(test_result_arr, test_result_arr);
//  // mean
//  Mean(test_result_arr, test_result);
//
//  // the function we're testing
//  fetch::ml::ops::MeanSquareError(y_arr, y_hat_arr, result);
//
//  ASSERT_TRUE(result == test_result);
//}
