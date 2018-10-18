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
#include "ml/layers/layers.hpp"
#include "ml/ops/ops.hpp"
#include "ml/session.hpp"

using namespace fetch::ml;

using Type            = double;
using ArrayType       = fetch::math::linalg::Matrix<Type>;
using VariableType    = fetch::ml::Variable<ArrayType>;
using VariablePtrType = std::shared_ptr<VariableType>;
using LayerType       = fetch::ml::layers::Layer<ArrayType>;
using LayerPtrType    = std::shared_ptr<LayerType>;

// we use pre-saved random weights so as not to risk a failure to pass the test
void AssignRandomWeights_1(LayerPtrType const &l1)
{
  l1->weights()->data()[0]  = 0.399209;
  l1->weights()->data()[1]  = -0.189532;
  l1->weights()->data()[2]  = -0.0395383;
  l1->weights()->data()[3]  = -0.149267;
  l1->weights()->data()[4]  = -0.0872096;
  l1->weights()->data()[5]  = 0.0887755;
  l1->weights()->data()[6]  = -0.145978;
  l1->weights()->data()[7]  = 0.098175;
  l1->weights()->data()[8]  = 0.481496;
  l1->weights()->data()[9]  = -0.428077;
  l1->weights()->data()[10] = -0.12723;
  l1->weights()->data()[11] = 0.537643;
  l1->weights()->data()[12] = 0.140156;
  l1->weights()->data()[13] = -0.410315;
  l1->weights()->data()[14] = 0.215899;
  l1->weights()->data()[15] = -0.0359184;
  l1->weights()->data()[16] = -0.0113837;
  l1->weights()->data()[17] = -0.583028;
  l1->weights()->data()[18] = -0.0108729;
  l1->weights()->data()[19] = -0.0719836;
  l1->weights()->data()[20] = -0.4012;
  l1->weights()->data()[21] = 0.186611;
  l1->weights()->data()[22] = -0.070605;
  l1->weights()->data()[23] = 0.146949;
  l1->weights()->data()[24] = -0.174336;
  l1->weights()->data()[25] = 0.0917895;
  l1->weights()->data()[26] = -0.0147523;
  l1->weights()->data()[27] = -0.0989468;
  l1->weights()->data()[28] = -0.484802;
  l1->weights()->data()[29] = 0.426212;
  l1->weights()->data()[30] = -0.0558087;
  l1->weights()->data()[31] = -0.540697;
  l1->weights()->data()[32] = 0.0521032;
  l1->weights()->data()[33] = 0.404141;
  l1->weights()->data()[34] = -0.217584;
  l1->weights()->data()[35] = -0.00498477;
  l1->weights()->data()[36] = -0.00696847;
  l1->weights()->data()[37] = 0.575511;
  l1->weights()->data()[38] = -0.0353656;
  l1->weights()->data()[39] = -0.136167;
}

// we use pre-saved random weights so as not to risk a failure to pass the test
void AssignRandomWeights_2(LayerPtrType const &y_pred)
{
  y_pred->weights()->data()[0]  = 0.564778;
  y_pred->weights()->data()[1]  = 0.255206;
  y_pred->weights()->data()[2]  = -0.0252777;
  y_pred->weights()->data()[3]  = 0.198224;
  y_pred->weights()->data()[4]  = -0.114401;
  y_pred->weights()->data()[5]  = -0.144575;
  y_pred->weights()->data()[6]  = -0.0472394;
  y_pred->weights()->data()[7]  = 0.129313;
  y_pred->weights()->data()[8]  = 0.643188;
  y_pred->weights()->data()[9]  = 0.593934;
  y_pred->weights()->data()[10] = -0.0403538;
  y_pred->weights()->data()[11] = 0.739467;
  y_pred->weights()->data()[12] = 0.137544;
  y_pred->weights()->data()[13] = 0.567428;
  y_pred->weights()->data()[14] = 0.269416;
  y_pred->weights()->data()[15] = 0.0254497;
  y_pred->weights()->data()[16] = -0.203794;
  y_pred->weights()->data()[17] = 0.793605;
  y_pred->weights()->data()[18] = -0.156907;
  y_pred->weights()->data()[19] = 0.0607288;
}

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
  std::size_t                             n_reps = 200;

  // set up some variables
  std::size_t data_points = 4;
  std::size_t input_size  = 2;
  std::size_t h1_size     = 20;
  std::size_t output_size = 1;

  std::vector<std::size_t> input_shape{data_points, input_size};
  std::vector<std::size_t> gt_shape{data_points, output_size};

  auto input_data = sess.Variable(input_shape, "Input_data");
  auto l1         = sess.Layer(input_size, h1_size, "LeakyRelu", "layer_1");
  sess.SetInput(l1, input_data);
  auto y_pred = sess.Layer(h1_size, output_size, "LeakyRelu", "output_layer");
  sess.SetInput(y_pred, l1->output());
  auto gt = sess.Variable(gt_shape, "GroundTruth");
  AssignRandomWeights_1(l1);
  AssignRandomWeights_2(y_pred);

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
  Type                                    alpha  = 0.2;
  std::size_t                             n_reps = 200;

  // set up some variables
  std::size_t data_points = 4;
  std::size_t input_size  = 2;
  std::size_t h1_size     = 20;
  std::size_t output_size = 2;

  std::vector<std::size_t> input_shape{data_points, input_size};
  std::vector<std::size_t> gt_shape{data_points, output_size};

  auto input_data = sess.Variable(input_shape, "Input_data");
  auto l1         = sess.Layer(input_size, h1_size, "LeakyRelu", "layer_1");
  sess.SetInput(l1, input_data);
  auto y_pred = sess.Layer(h1_size, output_size, "", "output_layer");
  sess.SetInput(y_pred, l1->output());
  auto gt = sess.Variable(gt_shape, "GroundTruthOnehot");
  //  AssignRandomWeights_1(l1);
  //  AssignRandomWeights_2(y_pred);

  SetInputXOR(input_data->data());

  gt->data().Set(0, 0, 1.0);  // Off
  gt->data().Set(0, 1, 0.0);

  gt->data().Set(1, 0, 0.0);  // On
  gt->data().Set(1, 1, 1.0);

  gt->data().Set(2, 0, 0.0);  // On
  gt->data().Set(2, 1, 1.0);

  gt->data().Set(3, 0, 1.0);  // Off
  gt->data().Set(3, 1, 0.0);

  // loss
  auto loss = fetch::ml::ops::SoftmaxCrossEntropyLoss(y_pred->output(), gt, sess);

  // backward pass to get gradient
  sess.BackProp(input_data, loss, alpha, n_reps);

  // forward pass on the computational graph
  auto prediction = sess.Predict(input_data, y_pred->output());

  std::cout << "loss->data()[0]: " << loss->data()[0] << std::endl;
  for (std::size_t idx = 0; idx < prediction.size(); ++idx)
  {
    std::cout << "prediction[" << idx << "]: " << prediction[idx] << std::endl;
  }

  ASSERT_TRUE(loss->data()[0] < 1.0);
  ASSERT_TRUE(prediction.AllClose(gt->data()));
  //  ASSERT_TRUE(prediction[0] < 0.1);
  //  ASSERT_TRUE(prediction[1] > 0.9);
  //  ASSERT_TRUE(prediction[2] > 0.9);
  //  ASSERT_TRUE(prediction[3] < 0.1);
}