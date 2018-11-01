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
#include <cmath>
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

using namespace fetch::ml;
#define ARRAY_SIZE 100

using Type            = double;
using ArrayType       = fetch::math::linalg::Matrix<Type>;
using VariableType    = fetch::ml::Variable<ArrayType>;
using VariablePtrType = std::shared_ptr<VariableType>;

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
  SessionManager<ArrayType> sess{};

  // set up some variables
  std::vector<std::size_t> l1_shape{2, 3};
  std::vector<std::size_t> l2_shape{3, 4};
  VariablePtrType          l1 = sess.Variable(l1_shape, "l1_input");
  VariablePtrType          l2 = sess.Variable(l2_shape, "l2_input");
  AssignVariableIncrement(l1, 1.0);
  AssignVariableIncrement(l2, 1.0);

  // Dot product
  auto y_pred = fetch::ml::ops::Dot(l1, l2, sess);

  // test shape
  ASSERT_TRUE(y_pred->shape()[0] == l1_shape[0]);
  ASSERT_TRUE(y_pred->shape()[1] == l2_shape[1]);

  // assign ground truth
  std::vector<Type> gt_vec{38, 44, 50, 56, 83, 98, 113, 128};
  ArrayType         gt{y_pred->shape()};
  AssignArray(gt, gt_vec);

  // Forward Prop & test
  auto prediction = sess.Predict(l1, y_pred);
  ASSERT_TRUE(prediction.AllClose(gt));

  // BackProp & test gradients
  sess.BackProp(l1, y_pred, 0.1);
  ArrayType gt_grad{y_pred->grad().shape()};
  AssignArray(gt_grad, 1.0);
  ASSERT_TRUE(y_pred->grad().AllClose(gt_grad));

  // Assign and Dot a new variable
  std::vector<std::size_t> l3_shape{4, 7};
  auto                     l3 = sess.Variable(l3_shape);
  AssignVariableIncrement(l3);
  auto y_pred_2 = fetch::ml::ops::Dot(y_pred, l3, sess);

  // BackProp
  sess.BackProp(l1, y_pred_2, 0.1);

  // generate ground truth for gradients
  ArrayType gt_grad2{y_pred_2->grad().shape()};
  AssignArray(gt_grad2, 1.0);
  ASSERT_TRUE(y_pred_2->grad().AllClose(gt_grad2));
}

TEST(session_test, trivial_backprop_relu_test)
{
  // set up session
  SessionManager<ArrayType, VariableType> sess{};

  std::size_t data_points = 1;
  std::size_t input_size  = 1;
  std::size_t h1_size     = 1;
  std::size_t output_size = 1;

  Type        alpha  = 0.01;
  std::size_t n_reps = 1000;

  // set up some variables
  std::vector<std::size_t> input_shape{data_points, input_size};   // data points x input size
  std::vector<std::size_t> weights_shape{input_size, h1_size};     // input size x neurons
  std::vector<std::size_t> biases_shape{1, h1_size};               // input size x neurons
  std::vector<std::size_t> weights_shape_2{h1_size, output_size};  // input size x neurons
  std::vector<std::size_t> gt_shape{data_points, output_size};     // data points x output size

  auto input_data = sess.Variable(input_shape, "input_data", false);
  auto weights    = sess.Variable(weights_shape, "weights", true);
  auto biases     = sess.Variable(biases_shape, "biases", true);
  auto gt         = sess.Variable(gt_shape, "gt", false);

  input_data->data().Set(0, 0, 0.001);
  weights->data().Set(0, 0.1);
  biases->data().Fill(0.0);
  gt->data().Set(0, 0, 1.0);

  // Dot product
  auto dot_1  = fetch::ml::ops::Dot(input_data, weights, sess);
  auto add_1  = fetch::ml::ops::AddBroadcast(dot_1, biases, sess);
  auto y_pred = fetch::ml::ops::Relu(add_1, sess);

  // define loss
  auto loss = fetch::ml::ops::MeanSquareError(y_pred, gt, sess);

  // BackProp
  sess.BackProp(input_data, loss, alpha, n_reps);
  ASSERT_TRUE(loss->data()[0] < 1.0);

  // make one prediction
  auto prediction = sess.Predict(input_data, y_pred);
  ASSERT_TRUE(prediction.AllClose(gt->data(), 0.1));
}

TEST(session_test, trivial_backprop_sigmoid_test)
{
  // set up session
  SessionManager<ArrayType, VariableType> sess{};

  Type        alpha  = 0.02;
  std::size_t n_reps = 1000;

  // set up some variables
  std::size_t              data_points = 1;
  std::size_t              input_size  = 1;
  std::size_t              h1_size     = 10;
  std::size_t              output_size = 1;
  std::vector<std::size_t> input_shape{data_points, input_size};   // data points x input size
  std::vector<std::size_t> weights_shape{input_size, h1_size};     // input size x neurons
  std::vector<std::size_t> biases_shape{1, h1_size};               // input size x neurons
  std::vector<std::size_t> weights_shape_2{h1_size, output_size};  // input size x neurons
  std::vector<std::size_t> gt_shape{data_points, output_size};     // data points x output size

  auto input_data = sess.Variable(input_shape, "input_data", false);
  auto weights    = sess.Variable(weights_shape, "weights", true);
  auto biases     = sess.Variable(biases_shape, "biases", true);
  auto weights2   = sess.Variable(weights_shape_2, "weights", true);
  auto gt         = sess.Variable(gt_shape, "gt", false);

  input_data->data().Set(0, 0, 0.001);
  AssignVariableIncrement(weights, -0.55, 0.1);
  biases->data().Fill(0.0);
  AssignVariableIncrement(weights, -0.55, 0.1);
  gt->data().Set(0, 0, 1.0);

  // Dot product
  auto dot_1  = fetch::ml::ops::Dot(input_data, weights, sess);
  auto add_1  = fetch::ml::ops::AddBroadcast(dot_1, biases, sess);
  auto sig_1  = fetch::ml::ops::Sigmoid(add_1, sess);
  auto y_pred = fetch::ml::ops::Dot(sig_1, weights2, sess);

  // define loss
  auto loss = fetch::ml::ops::MeanSquareError(y_pred, gt, sess);

  // BackProp
  sess.BackProp(input_data, loss, alpha, n_reps);
  ASSERT_TRUE(loss->data()[0] < 1.0);

  // make one prediction
  auto prediction = sess.Predict(input_data, y_pred);
  ASSERT_TRUE(prediction.AllClose(gt->data(), 0.1));
}