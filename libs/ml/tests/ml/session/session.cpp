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

#include "ml/session.hpp"
#include "math/linalg/matrix.hpp"
#include "ml/ops/ops.hpp"
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

using namespace fetch::ml;
#define ARRAY_SIZE 100

using Type            = double;
using ArrayType       = fetch::math::linalg::Matrix<Type>;
using VariableType    = fetch::ml::Variable<ArrayType>;
using VariablyPtrType = std::shared_ptr<VariableType>;

void AssignVariableIncrement(VariablyPtrType var, Type val = 0.0, Type incr = 1.0)
{
  for (std::size_t i = 0; i < var->shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < var->shape()[1]; ++j)
    {
      var->Set(i, j, val);
      val += incr;
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

TEST(session_test, trivial_backprop_test)
{

  // set up session
  SessionManager<ArrayType, VariableType> sess{};

  // set up some variables
  std::vector<std::size_t> l1_shape{2, 3};
  std::vector<std::size_t> l2_shape{3, 4};
  auto                     l1 = sess.Variable(l1_shape, "l1_input");
  auto                     l2 = sess.Variable(l2_shape, "l2_input");
  AssignVariableIncrement(l1, 1.0);
  AssignVariableIncrement(l2, 1.0);

  // Dot product
  auto ret = fetch::ml::ops::Dot(l1, l2, sess);

  // test shape
  ASSERT_TRUE(ret->shape()[0] == l1_shape[0]);
  ASSERT_TRUE(ret->shape()[1] == l2_shape[1]);

  // assign ground truth
  std::vector<Type> gt_vec{38, 44, 50, 56, 83, 98, 113, 128};
  ArrayType         gt{ret->shape()};
  AssignArray(gt, gt_vec);

  // Forward Prop
  sess.Forward(l1, ret->variable_name());

  // test correct values
  ASSERT_TRUE(ret->data().AllClose(gt));

  // BackProp
  sess.BackwardGraph(ret);

  // generate ground truth for gradients
  ArrayType gt_grad{ret->grad().shape()};
  AssignArray(gt_grad, 1.0);

  // test gradients
  ASSERT_TRUE(ret->grad().AllClose(gt_grad));

  // Assign and Dot a new variable
  std::vector<std::size_t> l3_shape{4, 7};
  auto                     l3 = sess.Variable(l3_shape);
  //  Variable<ArrayType> l3{sess, l3_shape};
  AssignVariableIncrement(l3);
  auto ret2 = fetch::ml::ops::Dot(ret, l3, sess);

  // BackProp
  sess.BackwardGraph(ret2);

  // generate ground truth for gradients
  ArrayType gt_grad2{ret2->grad().shape()};
  AssignArray(gt_grad2, 1.0);
  ASSERT_TRUE(ret2->grad().AllClose(gt_grad2));
}

TEST(session_test, dot_relu_backprop_test)
{

  // set up session
  SessionManager<ArrayType, VariableType> sess{};

  std::size_t data_points = 7;
  std::size_t input_size  = 10;
  std::size_t h1_size     = 20;
  std::size_t output_size = 1;

  Type        alpha  = 0.5;
  std::size_t n_reps = 25;

  // set up some variables
  std::vector<std::size_t> input_data_shape{data_points,
                                            input_size};            // 7 data points x 10 input size
  std::vector<std::size_t> l1_weights_shape{input_size, h1_size};   // 10 input size x 15 neurons
  std::vector<std::size_t> l2_weights_shape{h1_size, output_size};  // 15 neurons x 2 output neurons
  std::vector<std::size_t> gt_shape{data_points, output_size};      //

  auto input_data = sess.Variable(input_data_shape, "Input_X");
  auto l1_weights = sess.Variable(l1_weights_shape, "l1_weights");
  auto l2_weights = sess.Variable(l2_weights_shape, "l2_weights");
  auto gt         = sess.Variable(gt_shape, "GroundTruth");

  AssignVariableIncrement(input_data, 0.5, 0.01);
  AssignVariableIncrement(l1_weights, 0.01, -0.01);
  AssignVariableIncrement(l2_weights, 0.03, 0.01);
  AssignVariableIncrement(gt, 10.0, 0.2);

  // 1 layer
  auto ret  = fetch::ml::ops::Dot(input_data, l1_weights, sess);
  auto ret2 = fetch::ml::ops::Relu(ret, sess);

  // 2 layer
  auto ret3   = fetch::ml::ops::Dot(ret2, l2_weights, sess);
  auto y_pred = fetch::ml::ops::Relu(ret3, sess);

  // loss fn
  auto ret5 = fetch::ml::ops::MeanSquareError(y_pred, gt, sess);

  sess.BackProp(input_data, ret5, alpha, n_reps);

  // generate ground truth for gradients
  ASSERT_TRUE(y_pred->data().AllClose(gt->data()));
}