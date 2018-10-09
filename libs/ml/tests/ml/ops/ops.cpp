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

#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

#include "math/linalg/matrix.hpp"
#include "ml/ops/activation_functions.hpp"
#include "ml/ops/ops.hpp"
#include "ml/session.hpp"

using namespace fetch::ml;
#define ARRAY_SIZE 100

using Type            = double;
using ArrayType       = fetch::math::linalg::Matrix<Type>;
using VariableType    = fetch::ml::Variable<ArrayType>;
using VariablePtrType = std::shared_ptr<VariableType>;

void AssignRandom(VariablePtrType var, Type mean = 0.0, Type variance = 1.0)
{
  std::random_device         rd{};
  std::mt19937               gen{rd()};
  std::normal_distribution<> d{mean, std::sqrt(variance)};
  for (std::size_t i = 0; i < var->shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < var->shape()[1]; ++j)
    {
      var->Set(i, j, d(gen));
    }
  }
}
void AssignVariableIncrement(VariablePtrType var, Type val = 0.0, Type incr = 1.0)
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

void AssignBiasesIncrement(VariablePtrType bias, Type val = 1.0, Type incr = 1.0)
{
  for (std::size_t j = 0; j < bias->shape()[1]; ++j)
  {
    for (std::size_t i = 0; i < bias->shape()[0]; ++i)
    {
      bias->Set(i, j, val);
    }
    val += incr;
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

TEST(loss_functions, forward_dot_test)
{
  // set up session
  SessionManager<ArrayType, VariableType> sess{};

  // set up some variables
  std::vector<std::size_t> l1shape_{2, 3};
  std::vector<std::size_t> l2shape_{3, 4};
  auto                     l1 = sess.Variable(l1shape_, "l1");
  auto                     l2 = sess.Variable(l2shape_, "l2");
  AssignVariableIncrement(l1, 1.0);
  AssignVariableIncrement(l2, 1.0);

  // Dot product
  auto ret = fetch::ml::ops::Dot(l1, l2, sess);

  // forward pass on the computational graph
  sess.Forward(l1, ret);

  // test shape
  ASSERT_TRUE(ret->shape()[0] == l1shape_[0]);
  ASSERT_TRUE(ret->shape()[1] == l2shape_[1]);

  // assign ground truth
  std::vector<Type> gt_vec{38, 44, 50, 56, 83, 98, 113, 128};
  ArrayType         gt{ret->shape()};
  AssignArray(gt, gt_vec);

  // test correct values
  ASSERT_TRUE(ret->data().AllClose(gt));
}

TEST(loss_functions, Relu_test)
{

  SessionManager<ArrayType, VariableType> sess{};

  std::vector<std::size_t> l1shape{2, 3};

  auto l1 = sess.Variable(l1shape);
  AssignVariableIncrement(l1, -3.);

  auto ret = fetch::ml::ops::Relu(l1, sess);

  // forward pass on the computational graph
  sess.Forward(l1, ret);

  ASSERT_TRUE(ret->shape() == l1->shape());

  std::vector<Type> gt_vec{0, 0, 0, 0, 1, 2};
  ArrayType         gt{ret->shape()};
  AssignArray(gt, gt_vec);

  // test correct values
  ASSERT_TRUE(ret->data().AllClose(gt));
}

TEST(loss_functions, Sigmoid_test)
{

  SessionManager<ArrayType, VariableType> sess{};
  std::vector<std::size_t>                l1shape_{2, 3};

  auto l1 = sess.Variable(l1shape_);
  AssignVariableIncrement(l1, -3.);

  auto ret = fetch::ml::ops::Sigmoid(l1, sess);

  // forward pass on the computational graph
  sess.Forward(l1, ret);

  ASSERT_TRUE(ret->shape() == l1->shape());

  std::vector<Type> gt_vec{0.04742587317756678087885, 0.1192029220221175559403,
                           0.2689414213699951207488,  0.5,
                           0.7310585786300048792512,  0.8807970779778824440597};
  ArrayType         gt{ret->shape()};
  AssignArray(gt, gt_vec);

  // test correct values
  ASSERT_TRUE(ret->data().AllClose(gt));
}

TEST(loss_functions, Sum_test)
{

  SessionManager<ArrayType, VariableType> sess{};

  std::vector<std::size_t> l1shape{2, 3};

  auto l1 = sess.Variable(l1shape);
  AssignVariableIncrement(l1, 0.);

  auto ret = fetch::ml::ops::ReduceSum(l1, 1, sess);

  // forward pass on the computational graph
  sess.Forward(l1, ret);

  ASSERT_TRUE(ret->shape()[0] == l1->shape()[0]);
  ASSERT_TRUE(ret->shape()[1] == 1);

  std::vector<Type> gt_vec{3, 12};
  ArrayType         gt{ret->shape()};
  AssignArray(gt, gt_vec);

  ASSERT_TRUE(ret->data().AllClose(gt));
}

/**
 * The MSE is summed across data points (i.e. shape()[0]), but not across neurons (i.e. shape()[1])
 */
TEST(loss_functions, MSE_forward_test)
{

  SessionManager<ArrayType, VariableType> sess{};

  std::vector<std::size_t> shape{2, 3};

  auto l1 = sess.Variable(shape);
  auto l2 = sess.Variable(shape);
  auto gt = sess.Variable({1, 3});

  AssignVariableIncrement(l1, 0.1, 2.0);
  AssignVariableIncrement(l2, 1.2, 1.3);
  gt->data()[0] = 0.55249999999999999;
  gt->data()[1] = 0.76250000000000018;
  gt->data()[2] = 1.4625000000000004;

  auto mse = fetch::ml::ops::MeanSquareError(l1, l2, sess);

  // forward pass on the computational graph
  auto prediction = sess.Predict(l1, mse);

  ASSERT_TRUE(prediction.AllClose(gt->data()));
}

TEST(loss_functions, CEL_test)
{

  SessionManager<ArrayType, VariableType> sess{};

  std::vector<std::size_t> shape{3, 3};

  auto l1 = sess.Variable(shape);
  auto l2 = sess.Variable(shape);

  l1->Set(0, 0, 0.1);
  l1->Set(0, 1, 0.8);
  l1->Set(0, 2, 0.1);
  l1->Set(1, 0, 0.8);
  l1->Set(1, 1, 0.1);
  l1->Set(1, 2, 0.1);
  l1->Set(2, 0, 0.1);
  l1->Set(2, 1, 0.1);
  l1->Set(2, 2, 0.8);

  l2->Set(0, 0, 1);
  l2->Set(0, 1, 0);
  l2->Set(0, 2, 0);
  l2->Set(1, 0, 1);
  l2->Set(1, 1, 0);
  l2->Set(1, 2, 0);
  l2->Set(2, 0, 0);
  l2->Set(2, 1, 0);
  l2->Set(2, 2, 1);

  auto ret = fetch::ml::ops::CrossEntropyLoss(l1, l2, sess);

  // forward pass on the computational graph
  sess.Forward(l1, ret);

  ASSERT_TRUE(ret->shape()[0] == 1);
  ASSERT_TRUE(ret->shape()[1] == l1->shape()[1]);
  ASSERT_TRUE(ret->At(0, 0) == 0.84190954810275176);
  ASSERT_TRUE(ret->At(0, 1) == 0.0);
  ASSERT_TRUE(ret->At(0, 2) == 0.074381183771403236);
}

TEST(loss_functions, dot_add_backprop_test)
{
  // set up session
  SessionManager<ArrayType, VariableType> sess{};

  // set up some variables
  std::vector<std::size_t> input_shape{1, 2};
  std::vector<std::size_t> weights_shape{2, 3};
  std::vector<std::size_t> biases_shape{1, 3};
  std::vector<std::size_t> gt_shape{1, 3};

  auto input_data = sess.Variable(input_shape, "input_data");
  auto weights    = sess.Variable(weights_shape, "weights", true);
  auto biases     = sess.Variable(biases_shape, "biases", true);
  auto gt         = sess.Variable(gt_shape, "gt");

  AssignVariableIncrement(input_data, 1.0, 1.0);
  AssignVariableIncrement(weights, 0.1, 0.1);
  AssignVariableIncrement(biases, 0.98, 0.05);
  AssignVariableIncrement(gt, 2.0, 2.0);

  // Dot product
  auto dot_1  = fetch::ml::ops::Dot(input_data, weights, sess);
  auto y_pred = fetch::ml::ops::AddBroadcast(dot_1, biases, sess);

  // simple loss
  auto loss = fetch::ml::ops::MeanSquareError(y_pred, gt, sess);

  // forward pass on the computational graph
  auto prediction = sess.Predict(input_data, y_pred);

  // backward pass to get gradient
  sess.BackProp(input_data, loss, 0.01, 500);

  // forward pass on the computational graph
  prediction = sess.Predict(input_data, y_pred);

  ASSERT_TRUE(prediction.AllClose(gt->data()));
}

TEST(loss_functions, dot_relu_xor_test)
{
  // set up session
  SessionManager<ArrayType, VariableType> sess{};

  Type        alpha  = 0.1;
  std::size_t n_reps = 200;

  // set up some variables
  std::size_t              data_points = 4;
  std::size_t              input_size  = 2;
  std::size_t              h1_size     = 30;
  std::size_t              output_size = 1;
  std::vector<std::size_t> input_shape{data_points, input_size};
  std::vector<std::size_t> weights_shape{input_size, h1_size};
  std::vector<std::size_t> weights_shape2{h1_size, output_size};
  std::vector<std::size_t> gt_shape{data_points, output_size};

  auto input_data = sess.Variable(input_shape, "input_data", false);
  auto weights    = sess.Variable(weights_shape, "weights", true);
  auto weights2   = sess.Variable(weights_shape2, "weights", true);
  auto gt         = sess.Variable(gt_shape, "gt");

  input_data->data().Set(0, 0, 0.0);
  input_data->data().Set(0, 1, 0.0);
  input_data->data().Set(1, 0, 0.0);
  input_data->data().Set(1, 1, 1.0);
  input_data->data().Set(2, 0, 1.0);
  input_data->data().Set(2, 1, 0.0);
  input_data->data().Set(3, 0, 1.0);
  input_data->data().Set(3, 1, 1.0);

  gt->data().Set(0, 0, 0.0);
  gt->data().Set(1, 0, 1.0);
  gt->data().Set(2, 0, 1.0);
  gt->data().Set(3, 0, 0.0);

  AssignRandom(weights, 0.0, 1.0 / input_size);
  AssignRandom(weights2, 0.0, 1.0 / h1_size);

  // Dot product
  auto dot_1  = fetch::ml::ops::Dot(input_data, weights, sess);
  auto sig_1  = fetch::ml::ops::Relu(dot_1, sess);
  auto y_pred = fetch::ml::ops::Dot(sig_1, weights2, sess);

  // simple loss
  auto loss = fetch::ml::ops::MeanSquareError(y_pred, gt, sess);

  // forward pass on the computational graph
  auto prediction = sess.Predict(input_data, y_pred);

  // backward pass to get gradient
  sess.BackProp(input_data, loss, alpha, n_reps);
  ASSERT_TRUE(loss->data()[0] < 0.01);

  // forward pass on the computational graph
  prediction = sess.Predict(input_data, y_pred);

  ASSERT_TRUE(prediction.AllClose(gt->data(), 1.0));
}