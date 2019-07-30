//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "math/tensor.hpp"
#include "ml/graph.hpp"
#include "ml/layers/scaled_dot_product_attention.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/loss_functions.hpp"
#include "ml/ops/multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/subtract.hpp"
#include "ml/optimisation/adagrad_optimiser.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/optimisation/momentum_optimiser.hpp"
#include "ml/optimisation/rmsprop_optimiser.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"

#include "gtest/gtest.h"

template <typename T>
class OptimisersTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(OptimisersTest, MyTypes);

template <typename TypeParam>
std::shared_ptr<fetch::ml::Graph<TypeParam>> PrepareTestGraph(
    typename TypeParam::SizeType input_size, typename TypeParam::SizeType output_size,
    std::string &input_name, std::string &label_name, std::string &error_name)
{
  using SizeType = typename TypeParam::SizeType;

  SizeType hidden_size = SizeType(10);

  std::shared_ptr<fetch::ml::Graph<TypeParam>> g(std::make_shared<fetch::ml::Graph<TypeParam>>());

  input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});

  std::string fc1_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC1", {input_name}, input_size, hidden_size);
  std::string act_name    = g->template AddNode<fetch::ml::ops::Relu<TypeParam>>("", {fc1_name});
  std::string output_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC2", {act_name}, hidden_size, output_size);

  label_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});

  error_name = g->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "Error", {output_name, label_name});

  return g;
}

template <typename TypeParam>
void PrepareTestDataAndLabels1D(TypeParam &data, TypeParam &gt)
{
  using DataType = typename TypeParam::Type;

  data.Resize({1, 4});
  data.Set(0, 0, static_cast<DataType>(1));
  data.Set(0, 1, DataType(2));
  data.Set(0, 2, DataType(3));
  data.Set(0, 3, DataType(4));

  gt.Resize({1, 4});
  gt.Set(0, 0, DataType(2));
  gt.Set(0, 1, DataType(3));
  gt.Set(0, 2, DataType(4));
  gt.Set(0, 3, DataType(5));
}

template <typename TypeParam>
void PrepareTestDataAndLabels2D(TypeParam &data, TypeParam &gt)
{
  using DataType = typename TypeParam::Type;

  data.Resize({2, 2, 3});
  data.Set(0, 0, 0, static_cast<DataType>(1));
  data.Set(0, 1, 0, DataType(2));
  data.Set(1, 0, 0, DataType(3));
  data.Set(1, 1, 0, DataType(4));

  data.Set(0, 0, 1, DataType(5));
  data.Set(0, 1, 1, DataType(6));
  data.Set(1, 0, 1, DataType(7));
  data.Set(1, 1, 1, DataType(8));

  data.Set(0, 0, 2, DataType(9));
  data.Set(0, 1, 2, DataType(10));
  data.Set(1, 0, 2, DataType(11));
  data.Set(1, 1, 2, DataType(12));

  gt.Resize({2, 3});
  gt.Set(0, 0, DataType(2));
  gt.Set(1, 0, DataType(3));

  gt.Set(0, 1, DataType(6));
  gt.Set(1, 1, DataType(7));

  gt.Set(0, 2, DataType(10));
  gt.Set(1, 2, DataType(11));
}

TYPED_TEST(OptimisersTest, sgd_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.4f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(1, 1, input_name, label_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels1D(data, gt);

  // Initialise Optimiser
  fetch::ml::optimisers::SGDOptimiser<TypeParam> optimiser(g, {input_name}, label_name, output_name,
                                                           learning_rate);

  // Do 2 optimiser steps
  optimiser.Run({data}, gt);
  DataType loss = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 0.459031165,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 1000.0);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.019647587090730667,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.1836218386888504,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.08435,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.014735775999724865,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, sgd_optimiser_training_2D)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.06f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(4, 2, input_name, label_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels2D(data, gt);

  // Initialise Optimiser
  fetch::ml::optimisers::SGDOptimiser<TypeParam> optimiser(g, {input_name}, label_name, output_name,
                                                           learning_rate);

  // Do 2 optimiser steps
  optimiser.Run({data}, gt);
  DataType loss = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 364.65829467773438,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 100000.0);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), -0.23445655405521393,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -4.0157435419938992,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), -0.41828477382659912,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -4.4603282147928267,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, momentum_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.16f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(1, 1, input_name, label_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels1D(data, gt);

  // Initialise Optimiser
  fetch::ml::optimisers::MomentumOptimiser<TypeParam> optimiser(g, {input_name}, label_name,
                                                                output_name, learning_rate);

  // Do 2 optimiser steps to ensure that momentum was applied
  optimiser.Run({data}, gt);
  DataType loss = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 0.279862642,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 1000.0);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.056329719722270966,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.1836218386888504,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.14914020895957947,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.014735775999724865,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, momentum_optimiser_training_2D)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.06f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(4, 2, input_name, label_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels2D(data, gt);

  // Initialise Optimiser
  fetch::ml::optimisers::MomentumOptimiser<TypeParam> optimiser(g, {input_name}, label_name,
                                                                output_name, learning_rate);

  // Do 2 optimiser steps
  optimiser.Run({data}, gt);
  DataType loss = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 364.658295,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 10000.0);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), -0.19961071014404297,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -3.6191232204437256,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), -0.31931310892105103,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -3.7157034873962402,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, adagrad_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.04f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(1, 1, input_name, label_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels1D(data, gt);

  // Initialise Optimiser
  fetch::ml::optimisers::AdaGradOptimiser<TypeParam> optimiser(g, {input_name}, label_name,
                                                               output_name, learning_rate);

  // Do multiple steps
  optimiser.Run({data}, gt);
  DataType loss = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 0.511220098,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 1000.0);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.06322682648897171,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.1836218386888504,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.061630338430404663,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.014735775999724865,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, adagrad_optimiser_training_2D)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.04f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(4, 2, input_name, label_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels2D(data, gt);

  // Initialise Optimiser
  fetch::ml::optimisers::AdaGradOptimiser<TypeParam> optimiser(g, {input_name}, label_name,
                                                               output_name, learning_rate);

  // Do multiple steps
  optimiser.Run({data}, gt);
  DataType loss = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 4.52624369,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 1000.0);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.06218932569026947,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.102255,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.061548389494419098,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.1116119846701622,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, rmsprop_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.01f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(1, 1, input_name, label_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels1D(data, gt);

  // Initialise Optimiser
  fetch::ml::optimisers::RMSPropOptimiser<TypeParam> optimiser(g, {input_name}, label_name,
                                                               output_name, learning_rate);

  // Do multiple steps
  optimiser.Run({data}, gt);
  DataType loss = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 0.646417379,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 1000.0);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.05176149308681488,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.1836218386888504,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.050760529935359955,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.014735775999724865,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, rmsprop_optimiser_training_2D)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.01f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(4, 2, input_name, label_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels2D(data, gt);

  // Initialise Optimiser
  fetch::ml::optimisers::RMSPropOptimiser<TypeParam> optimiser(g, {input_name}, label_name,
                                                               output_name, learning_rate);

  // Do multiple steps
  optimiser.Run({data}, gt);
  DataType loss = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 6.06429482,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 1000.0);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.051881901919841766,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.11241874098777771,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.05076,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.12340624630451202,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, adam_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.01f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(1, 1, input_name, label_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels1D(data, gt);

  // Initialise Optimiser
  fetch::ml::optimisers::AdamOptimiser<TypeParam> optimiser(g, {input_name}, label_name,
                                                            output_name, learning_rate);

  // Do multiple steps
  optimiser.Run({data}, gt);
  DataType loss = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 1.0529532, 1e-2);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.021604862064123154,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.1836218386888504,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.021597184240818024,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.014735775999724865,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, adam_optimiser_training_2D)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.01f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(4, 2, input_name, label_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels2D(data, gt);

  // Initialise Optimiser
  fetch::ml::optimisers::AdamOptimiser<TypeParam> optimiser(g, {input_name}, label_name,
                                                            output_name, learning_rate);

  // Do multiple steps
  optimiser.Run({data}, gt);
  DataType loss = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 10.957704, 1e-2);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.021601308137178421,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.14116032421588898,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.021602753549814224,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.1541752964258194,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, adam_optimiser_minibatch_training)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.01f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =

      PrepareTestGraph<TypeParam>(1, 1, input_name, label_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels1D(data, gt);

  // Initialise Optimiser
  fetch::ml::optimisers::AdamOptimiser<TypeParam> optimiser(g, {input_name}, label_name,
                                                            output_name, learning_rate);

  // Do multiple steps
  optimiser.Run({data}, gt, 3);
  DataType loss = optimiser.Run({data}, gt, 2);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 1.2803993316525915, 1e-5);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.050698753797164819,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.1836218386888504,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.050653,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.014735775999724865,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}
