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
using VariableType = fetch::ml::Variable<ArrayType>;

void AssignVariableIncrement(VariableType &var, Type val = 0.0)
{
  for (std::size_t i = 0; i < var.shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < var.shape()[1]; ++j)
    {
      var.Set(i, j, val);
      ++val;
    }
  }
}
void AssignArray(ArrayType &var, Type val = 1.0)
{
  for (std::size_t i = 0; i < var.shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < var.shape()[1]; ++j)
    {
      var.Set(i, j, val);
    }
  }
}
void AssignArray(ArrayType &var, std::vector<Type> vec_val)
{
  std::size_t counter = 0;
  for (std::size_t i = 0; i < var.shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < var.shape()[1]; ++j)
    {
      var.Set(i, j, vec_val[counter]);
      ++counter;
    }
  }
}

TEST(loss_functions, Dot_test)
{
  // set up session
  SessionManager<ArrayType, VariableType> sess{};

  // set up some variables
  std::vector<std::size_t> l1_shape{2, 3};
  std::vector<std::size_t> l2_shape{3, 4};
  Variable<ArrayType> l1{sess, l1_shape};
  Variable<ArrayType> l2{sess, l2_shape};
  AssignVariableIncrement(l1, 1.0);
  AssignVariableIncrement(l2, 1.0);

  // Dot product
  Variable<ArrayType> ret = fetch::ml::ops::Dot(l1, l2, sess);

  // test shape
  ASSERT_TRUE(ret.shape()[0] == l1_shape[0]);
  ASSERT_TRUE(ret.shape()[1] == l2_shape[1]);

  // assign ground truth
  std::vector<Type> gt_vec{38, 44, 50, 56, 83, 98, 113, 128};
  ArrayType gt{ret.shape()};
  AssignArray(gt, gt_vec);

  // test correct values
  ASSERT_TRUE(ret.data().AllClose(gt));
}

TEST(loss_functions, Relu_test)
{

  SessionManager<ArrayType, VariableType> sess{};

  std::vector<std::size_t> l1_shape{2, 3};

  Variable<ArrayType> l1{sess, l1_shape};
  AssignVariableIncrement(l1, -3.);

  Variable<ArrayType> ret = fetch::ml::ops::Relu(l1, sess);

  ASSERT_TRUE(ret.shape() == l1.shape());

  std::vector<Type> gt_vec{0, 0, 0, 0, 1, 2};
  ArrayType gt{ret.shape()};
  AssignArray(gt, gt_vec);

  // test correct values
  ASSERT_TRUE(ret.data().AllClose(gt));

}

TEST(loss_functions, Sigmoid_test)
{

  SessionManager<ArrayType, VariableType> sess{};
  std::vector<std::size_t> l1_shape{2, 3};

  Variable<ArrayType> l1{sess, l1_shape};
  AssignVariableIncrement(l1, -3.);

  Variable<ArrayType> ret = fetch::ml::ops::Sigmoid(l1, sess);

  ASSERT_TRUE(ret.shape() == l1.shape());

  std::vector<Type> gt_vec{0.95257412682243336, 0.88079707797788231, 0.7310585786300049, 0.5, 0.2689414213699951, 0.11920292202211755};
  ArrayType gt{ret.shape()};
  AssignArray(gt, gt_vec);

  // test correct values
  ASSERT_TRUE(ret.data().AllClose(gt));

}

TEST(loss_functions, Sum_test)
{

  SessionManager<ArrayType, VariableType> sess{};

  std::vector<std::size_t> l1_shape{2, 3};

  Variable<ArrayType> l1{sess, l1_shape};
  AssignVariableIncrement(l1, 0.);

  Variable<ArrayType> ret = fetch::ml::ops::Sum(l1, 1, sess);

  ASSERT_TRUE(ret.shape()[0] == l1.shape()[0]);
  ASSERT_TRUE(ret.shape()[1] == 1);

  std::vector<Type> gt_vec{3, 12};
  ArrayType gt{ret.shape()};
  AssignArray(gt, gt_vec);

  // test correct values
  ASSERT_TRUE(ret.data().AllClose(gt));

}

TEST(loss_functions, MSE_test)
{

  SessionManager<ArrayType, VariableType> sess{};

  std::vector<std::size_t> shape{2, 3};

  Variable<ArrayType> l1{sess, shape};
  Variable<ArrayType> l2{sess, shape};

  AssignVariableIncrement(l1, 0.);
  AssignVariableIncrement(l2, 1.);

  Variable<ArrayType> ret = fetch::ml::ops::MeanSquareError(l1, l2, sess);

  ASSERT_TRUE(ret.shape()[0] == 1);
  ASSERT_TRUE(ret.shape()[1] == 1);

  std::vector<Type> gt_vec{6};
  ArrayType gt{ret.shape()};
  AssignArray(gt, gt_vec);
}

TEST(loss_functions, CEL_test)
{

  SessionManager<ArrayType, VariableType> sess{};

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