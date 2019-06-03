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

#include "ml/layers/self_attention.hpp"

#include <gtest/gtest.h>

template <typename T>
class OptimisersTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(OptimisersTest, MyTypes);

template <typename TypeParam>
std::shared_ptr<fetch::ml::Graph<TypeParam>> PrepareTestGraph(std::string &input_name,
                                                              std::string &output_name)
{
  using SizeType = typename TypeParam::SizeType;

  SizeType input_size  = SizeType(1);
  SizeType output_size = SizeType(1);
  SizeType hidden_size = SizeType(10);

  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      std::shared_ptr<fetch::ml::Graph<TypeParam>>(new fetch::ml::Graph<TypeParam>());

  input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});

  std::string fc1_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC1", {input_name}, input_size, hidden_size);
  std::string act_name = g->template AddNode<fetch::ml::ops::Relu<TypeParam>>("", {fc1_name});
  output_name          = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC2", {act_name}, hidden_size, output_size);

  return g;
}

template <typename TypeParam>
void PrepareTestDataAndLabels(TypeParam &data, TypeParam &gt)
{
  using DataType = typename TypeParam::Type;

  data.Resize({4, 1});
  data.Set(0, 0, DataType(1));
  data.Set(1, 0, DataType(2));
  data.Set(2, 0, DataType(3));
  data.Set(3, 0, DataType(4));

  gt.Resize({4, 1});
  gt.Set(0, 0, DataType(2));
  gt.Set(1, 0, DataType(3));
  gt.Set(2, 0, DataType(4));
  gt.Set(3, 0, DataType(5));
}

TYPED_TEST(OptimisersTest, sgd_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.1f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(input_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels(data, gt);

  // Initialize Optimiser
  fetch::ml::optimisers::SGDOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
      optimiser(g, input_name, output_name, learning_rate);

  // Do 2 optimiser steps
  optimiser.Run(data, gt);
  DataType loss = optimiser.Run(data, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 1.83612, 1e-5);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();

  EXPECT_NEAR(static_cast<double>(weights[3].At(2, 0)), -0.01474, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.08435, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[0].At(0, 9)), 0.01965, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[1].At(0, 4)), -0.18362, 1e-5);
}

TYPED_TEST(OptimisersTest, momentum_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.04f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(input_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels(data, gt);

  // Initialize Optimiser
  fetch::ml::optimisers::MomentumOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
      optimiser(g, input_name, output_name, learning_rate);

  // Do 2 optimiser steps to ensure that momentum was applied
  optimiser.Run(data, gt);
  DataType loss = optimiser.Run(data, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 1.11945, 1e-5);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[3].At(2, 0)), -0.01474, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.14914, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[0].At(0, 9)), 0.05633, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[1].At(0, 4)), -0.18362, 1e-5);
}

TYPED_TEST(OptimisersTest, adagrad_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.04f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(input_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels(data, gt);

  // Initialize Optimiser
  fetch::ml::optimisers::AdaGradOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
      optimiser(g, input_name, output_name, learning_rate);

  // Do multiple steps
  optimiser.Run(data, gt);
  DataType loss = optimiser.Run(data, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 2.04488, 1e-5);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[3].At(2, 0)), -0.01474, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.06163, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[0].At(0, 9)), 0.06323, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[1].At(0, 4)), -0.18362, 1e-5);
}

TYPED_TEST(OptimisersTest, rmsprop_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.01f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(input_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels(data, gt);

  // Initialize Optimiser
  fetch::ml::optimisers::RMSPropOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
      optimiser(g, input_name, output_name, learning_rate);

  // Do multiple steps
  optimiser.Run(data, gt);
  DataType loss = optimiser.Run(data, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 2.58567, 1e-5);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[3].At(2, 0)), -0.01474, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.05076, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[0].At(0, 9)), 0.05176, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[1].At(0, 4)), -0.18362, 1e-5);
}

TYPED_TEST(OptimisersTest, adam_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.01f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(input_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels(data, gt);

  // Initialize Optimiser
  fetch::ml::optimisers::AdamOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
      optimiser(g, input_name, output_name, learning_rate);

  // Do multiple steps
  optimiser.Run(data, gt);
  DataType loss = optimiser.Run(data, gt);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 4.21160, 1e-5);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[3].At(2, 0)), -0.01474, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.02160, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[0].At(0, 9)), 0.02162, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[1].At(0, 4)), -0.18362, 1e-5);
}

TYPED_TEST(OptimisersTest, adam_optimiser_minibatch_training)
{
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.01f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      PrepareTestGraph<TypeParam>(input_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels(data, gt);

  // Initialize Optimiser
  fetch::ml::optimisers::AdamOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
      optimiser(g, input_name, output_name, learning_rate);

  // Do multiple steps
  optimiser.Run(data, gt, 3);
  DataType loss = optimiser.Run(data, gt, 2);

  // Test loss
  EXPECT_NEAR(static_cast<double>(loss), 1.33776, 1e-5);

  // Test weights
  std::vector<TypeParam> weights = g->get_weights();
  EXPECT_NEAR(static_cast<double>(weights[3].At(2, 0)), -0.01474, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.04817, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[0].At(0, 9)), 0.04839, 1e-5);
  EXPECT_NEAR(static_cast<double>(weights[1].At(0, 4)), -0.18362, 1e-5);
}
