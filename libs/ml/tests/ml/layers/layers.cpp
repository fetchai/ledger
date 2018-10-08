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
#include "ml/variable.hpp"

using namespace fetch::ml;

using Type            = double;
using ArrayType       = fetch::math::linalg::Matrix<Type>;
using VariableType    = fetch::ml::Variable<ArrayType>;
using VariablyPtrType = std::shared_ptr<VariableType>;
using LayerType       = fetch::ml::layers::Layer<ArrayType>;
using LayerPtrType    = std::shared_ptr<LayerType>;

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

TEST(layers_test, simple_arithmetic)
{

  // set up session
  SessionManager<ArrayType, Variable<ArrayType>> sess{};

  std::size_t data_points = 7;
  std::size_t input_size  = 10;
  std::size_t h1_size     = 20;
  std::size_t output_size = 1;

  Type        alpha  = 0.001;
  std::size_t n_reps = 500;

  // set up some variables
  std::vector<std::size_t> input_data_shape{data_points, input_size};
  std::vector<std::size_t> gt_shape{data_points, output_size};

  auto input_data = sess.Variable(input_data_shape, "Input_X");
  auto l1         = sess.Layer(input_size, h1_size);
  auto y_pred     = sess.Layer(h1_size, output_size);
  auto gt         = sess.Variable(gt_shape, "GroundTruth");

  AssignVariableIncrement(input_data, 1.0, 1.0);
  sess.SetInput(l1, input_data);
  sess.SetInput(y_pred, l1->output());
  AssignVariableIncrement(gt, 10.0, 10.0);

  auto loss = fetch::ml::ops::MeanSquareError(y_pred->output(), gt, sess);

  std::cout.precision(17);
  sess.BackProp(input_data, loss, alpha, n_reps);
  auto prediction = sess.Predict(input_data, y_pred->output());
  for (std::size_t idx = 0; idx < prediction.size(); ++idx)
  {
    std::cout << "prediction[" << idx << "]: " << prediction[idx] << std::endl;
  }
  for (std::size_t idx = 0; idx < y_pred->output()->grad().size(); ++idx)
  {
    std::cout << "y_pred->output()->grad()[" << idx << "]: " << y_pred->output()->grad()[idx]
              << std::endl;
  }
  for (std::size_t idx = 0; idx < y_pred->weights()->grad().size(); ++idx)
  {
    std::cout << "y_pred->weights()->grad()[" << idx << "]: " << y_pred->weights()->grad()[idx]
              << std::endl;
  }

  sess.BackProp(input_data, loss, alpha, n_reps);
  prediction = sess.Predict(input_data, y_pred->output());
  for (std::size_t idx = 0; idx < prediction.size(); ++idx)
  {
    std::cout << "prediction[" << idx << "]: " << prediction[idx] << std::endl;
  }
  sess.BackProp(input_data, loss, alpha, n_reps);
  prediction = sess.Predict(input_data, y_pred->output());
  for (std::size_t idx = 0; idx < prediction.size(); ++idx)
  {
    std::cout << "prediction[" << idx << "]: " << prediction[idx] << std::endl;
  }
  // generate ground truth for gradients
  ASSERT_TRUE(prediction.AllClose(gt->data()));
}

TEST(session_test, dot_relu_CEL_backprop_test)
{
  //
  //  // set up session
  //  SessionManager<ArrayType, VariableType> sess{};
  //
  //  std::size_t data_points = 4;
  //  std::size_t input_size  = 2;
  //  std::size_t h1_size     = 10;
  //  std::size_t output_size = 3;
  //
  //  Type        alpha  = 0.5;
  //  std::size_t n_reps = 25;
  //
  //  // set up some variables
  //  std::vector<std::size_t> input_data_shape{data_points,
  //                                            input_size};            // 7 data points x 10 input
  //                                            size
  //  std::vector<std::size_t> l1_weights_shape{input_size, h1_size};   // 10 input size x 15
  //  neurons std::vector<std::size_t> l2_weights_shape{h1_size, output_size};  // 15 neurons x 2
  //  output neurons std::vector<std::size_t> gt_shape{data_points, output_size};      //
  //
  //  auto input_data = sess.Variable(input_data_shape, "Input_X");
  //  auto l1 = sess.Layer(input_size, h1_size);
  //  auto l2 = sess.Layer(h1_size, output_size);
  //  auto gt         = sess.Variable(gt_shape, "GroundTruth");
  //
  //  input_data->Set(0, 0, 0.1);
  //  input_data->Set(0, 1, 0.6);
  //  input_data->Set(1, 0, 0.2);
  //  input_data->Set(1, 1, 0.7);
  //  input_data->Set(2, 0, 0.3);
  //  input_data->Set(2, 1, 0.8);
  //  input_data->Set(3, 0, 0.4);
  //  input_data->Set(3, 1, 0.9);
  //
  //  gt->Set(0, 0, 1.0);
  //  gt->Set(0, 1, 0.0);
  //  gt->Set(0, 2, 0.0);
  //  gt->Set(1, 0, 0.0);
  //  gt->Set(1, 1, 1.0);
  //  gt->Set(1, 2, 0.0);
  //  gt->Set(2, 0, 0.0);
  //  gt->Set(2, 1, 1.0);
  //  gt->Set(2, 2, 0.0);
  //  gt->Set(3, 0, 0.0);
  //  gt->Set(3, 1, 0.0);
  //  gt->Set(3, 2, 1.0);
  //
  //  sess.SetInput(l1, input_data);
  //  sess.SetInput(l2, l1->output());
  //
  //  auto y_pred = l2->output();
  //  auto ret    = fetch::ml::ops::CrossEntropyLoss(y_pred, gt, sess);
  //
  //  sess.BackProp(input_data, ret, alpha, n_reps);
  //
  //  auto prediction = sess.Predict(input_data, y_pred);
  //
  //  for (std::size_t idx = 0; idx < l1->dot_output()->data().size(); ++idx) {
  //    std::cout << "l1->dot_output()->data()[" << idx << "]: " << l1->dot_output()->data()[idx] <<
  //    std::endl;
  //  }
  //
  //
  //  for (std::size_t idx = 0; idx < l1->output()->data().size(); ++idx) {
  //    std::cout << "l1->output()->data()[" << idx << "]: " << l1->output()->data()[idx] <<
  //    std::endl;
  //  }
  //
  //  for (std::size_t idx = 0; idx < prediction.size(); ++idx) {
  //    std::cout << "prediction[" << idx << "]: " << prediction[idx] << std::endl;
  //  }
  //
  //
  //  for (std::size_t idx = 0; idx < ret->data().size(); ++idx) {
  //    std::cout << "ret->data()[" << idx << "]: " << ret->data()[idx] << std::endl;
  //  }
  //
  //
  //  // generate ground truth for gradients
  //  ASSERT_TRUE(prediction.AllClose(gt->data()));
}
