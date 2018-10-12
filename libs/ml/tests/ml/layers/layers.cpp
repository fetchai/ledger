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
#include "ml/computation_graph/computation_graph.hpp"
#include "ml/layers/layers.hpp"
#include "ml/ops/ops.hpp"
#include "ml/session.hpp"
//#include "ml/variable.hpp"

using namespace fetch::ml;

using Type            = double;
using ArrayType       = fetch::math::linalg::Matrix<Type>;
using VariableType    = fetch::ml::Variable<ArrayType>;
using VariablyPtrType = std::shared_ptr<VariableType>;
using LayerType       = fetch::ml::layers::Layer<ArrayType>;
using LayerPtrType    = std::shared_ptr<LayerType>;

void SetInputXOR(ArrayType &input_data)
{
  input_data.Set(0, 0, 0.0);
  input_data.Set(0, 1, 0.0);
  input_data.Set(1, 0, 0.0);
  input_data.Set(1, 1, 1.0);
  input_data.Set(2, 0, 1.0);
  input_data.Set(2, 1, 0.0);
  input_data.Set(3, 0, 1.0);
  input_data.Set(3, 1, 1.0);
}

TEST(layers_test, two_layer_xor_MSE)
{
  // set up session
  SessionManager<ArrayType, VariableType> sess{};
  Type                                    alpha  = 0.2;
  std::size_t                             n_reps = 500;

  // set up some variables
  std::size_t data_points = 4;
  std::size_t input_size  = 2;
  std::size_t h1_size     = 30;
  std::size_t output_size = 1;

  std::vector<std::size_t> input_shape{data_points, input_size};
  std::vector<std::size_t> gt_shape{data_points, output_size};

  auto input_data = sess.Variable(input_shape, "Input_data");
  auto l1         = sess.Layer(input_size, h1_size, "LeakyRelu", "layer_1");
  sess.SetInput(l1, input_data);
  auto y_pred = sess.Layer(h1_size, output_size, "", "output_layer");
  sess.SetInput(y_pred, l1->output());
  auto gt = sess.Variable(gt_shape, "GroundTruth");

  SetInputXOR(input_data->data());

  gt->data().Set(0, 0, 0.0);
  gt->data().Set(1, 0, 1.0);
  gt->data().Set(2, 0, 1.0);
  gt->data().Set(3, 0, 0.0);

  // loss
  auto loss = fetch::ml::ops::MeanSquareError(y_pred->output(), gt, sess);

  // backward pass to get gradient
  sess.BackProp(input_data, loss, alpha, n_reps);

  // forward pass on the computational graph
  auto prediction = sess.Predict(input_data, y_pred->output());
  ASSERT_TRUE(loss->data()[0] < 1.0);
  ASSERT_TRUE(prediction[0] < 0.1);
  ASSERT_TRUE(prediction[1] > 0.9);
  ASSERT_TRUE(prediction[2] > 0.9);
  ASSERT_TRUE(prediction[3] < 0.1);
}

TEST(layers_test, two_layer_xor_CEL)
{
  // set up session
  SessionManager<ArrayType, VariableType> sess{};

  Type        alpha  = 0.1;
  std::size_t n_reps = 500;

  // set up some variables
  std::size_t data_points = 4;
  std::size_t input_size  = 2;
  std::size_t h1_size     = 30;
  std::size_t output_size = 2;

  std::vector<std::size_t> input_shape{data_points, input_size};
  std::vector<std::size_t> gt_shape{data_points, output_size};

  auto input_data = sess.Variable(input_shape, "Input_data");
  auto l1         = sess.Layer(input_size, h1_size, "LeakyRelu", "layer_1");
  sess.SetInput(l1, input_data);
  auto y_pred = sess.Layer(h1_size, output_size, "", "output_layer");
  sess.SetInput(y_pred, l1->output());
  auto gt        = sess.Variable(gt_shape, "GroundTruth");
  auto gt_onehot = sess.Variable({data_points, 1}, "GroundTruth_onehot");

  SetInputXOR(input_data->data());

  gt->data().Set(0, 0, 1.0);
  gt->data().Set(0, 1, 0.0);
  gt->data().Set(1, 0, 0.0);
  gt->data().Set(1, 1, 1.0);
  gt->data().Set(2, 0, 0.0);
  gt->data().Set(2, 1, 1.0);
  gt->data().Set(3, 0, 1.0);
  gt->data().Set(3, 1, 0.0);

  gt_onehot->data().Set(0, 0.0);
  gt_onehot->data().Set(1, 1.0);
  gt_onehot->data().Set(2, 1.0);
  gt_onehot->data().Set(3, 0.0);

  // loss
  auto loss = fetch::ml::ops::CrossEntropyLoss(y_pred->output(), gt, sess);

  // backward pass to get gradient
  sess.BackProp(input_data, loss, alpha, n_reps);

  // forward pass on the computational graph
  auto prediction = sess.Predict(input_data, y_pred->output());
  ASSERT_TRUE(loss->data()[0] < 0.1);

  auto preds = fetch::math::ArgMax(prediction, 1);
  ASSERT_TRUE(preds.AllClose(gt_onehot->data()));
}