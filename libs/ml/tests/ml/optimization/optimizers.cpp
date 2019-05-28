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
#include "ml/optimization/adagrad_optimizer.hpp"
#include "ml/optimization/adam_optimizer.hpp"
#include "ml/optimization/momentum_optimizer.hpp"
#include "ml/optimization/rmsprop_optimizer.hpp"
#include "ml/optimization/sgd_optimizer.hpp"

#include "ml/layers/self_attention.hpp"

#include <gtest/gtest.h>

template <typename T>
class OptimizersTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(OptimizersTest, MyTypes);

TYPED_TEST(OptimizersTest, sgd_optimizer_training)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.01f};
  SizeType input_size    = SizeType(1);
  SizeType output_size   = SizeType(1);
  SizeType n_batches     = SizeType(3);
  SizeType hidden_size   = SizeType(100);

  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      std::shared_ptr<fetch::ml::Graph<TypeParam>>(new fetch::ml::Graph<TypeParam>());

  std::string x_input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});

  std::string fc1_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC1", {x_input_name}, input_size, hidden_size);
  std::string act_name    = g->template AddNode<fetch::ml::ops::Relu<TypeParam>>("", {fc1_name});
  std::string output_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC2", {act_name}, hidden_size, output_size);

  ////////////////////////////////////////
  /// DEFINING DATA AND LABELS FOR XOR ///
  ////////////////////////////////////////

  TypeParam data{{4, 1}};
  data.Set(0, 0, DataType(1));
  data.Set(1, 0, DataType(2));
  data.Set(2, 0, DataType(3));
  data.Set(3, 0, DataType(4));

  TypeParam gt{{4, 1}};
  gt.Set(0, 0, DataType(2));
  gt.Set(1, 0, DataType(3));
  gt.Set(2, 0, DataType(4));
  gt.Set(3, 0, DataType(5));

  //////////////////////
  /// Initialize SGD ///
  //////////////////////

  fetch::ml::SGDOptimizer<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>> optimizer(
      g, x_input_name, output_name, learning_rate);

  /////////////////////
  /// TRAINING LOOP ///
  /////////////////////

  DataType loss;
  for (std::size_t i = 0; i < n_batches; ++i)
  {
    loss = optimizer.DoBatch(data, gt);
  }
  EXPECT_NEAR(static_cast<double>(loss), 2.3003983497619629, 1e-5);
}

TYPED_TEST(OptimizersTest, momentum_optimizer_training)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  DataType learning_rate   = DataType{0.01f};
  DataType momentum_update = DataType{0.9f};
  SizeType input_size      = SizeType(1);
  SizeType output_size     = SizeType(1);
  SizeType n_batches       = SizeType(3);
  SizeType hidden_size     = SizeType(100);

  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      std::shared_ptr<fetch::ml::Graph<TypeParam>>(new fetch::ml::Graph<TypeParam>());

  std::string x_input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});

  std::string fc1_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC1", {x_input_name}, input_size, hidden_size);
  std::string act_name    = g->template AddNode<fetch::ml::ops::Relu<TypeParam>>("", {fc1_name});
  std::string output_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC2", {act_name}, hidden_size, output_size);

  ////////////////////////////////////////
  /// DEFINING DATA AND LABELS FOR XOR ///
  ////////////////////////////////////////

  TypeParam data{{4, 1}};
  data.Set(0, 0, DataType(1));
  data.Set(1, 0, DataType(2));
  data.Set(2, 0, DataType(3));
  data.Set(3, 0, DataType(4));

  TypeParam gt{{4, 1}};
  gt.Set(0, 0, DataType(2));
  gt.Set(1, 0, DataType(3));
  gt.Set(2, 0, DataType(4));
  gt.Set(3, 0, DataType(5));

  //////////////////////
  /// Initialize SGD ///
  //////////////////////

  fetch::ml::MomentumOptimizer<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>> optimizer(
      g, x_input_name, output_name, learning_rate, momentum_update);

  /////////////////////
  /// TRAINING LOOP ///
  /////////////////////

  DataType loss;
  for (std::size_t i = 0; i < n_batches; ++i)
  {
    loss = optimizer.DoBatch(data, gt);
  }
  EXPECT_NEAR(static_cast<double>(loss), 0.095217056572437286, 1e-5);
}

TYPED_TEST(OptimizersTest, adagrad_optimizer_training)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.01f};
  SizeType input_size    = SizeType(1);
  SizeType output_size   = SizeType(1);
  SizeType n_batches     = SizeType(3);
  SizeType hidden_size   = SizeType(100);

  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      std::shared_ptr<fetch::ml::Graph<TypeParam>>(new fetch::ml::Graph<TypeParam>());

  std::string x_input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});

  std::string fc1_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC1", {x_input_name}, input_size, hidden_size);
  std::string act_name    = g->template AddNode<fetch::ml::ops::Relu<TypeParam>>("", {fc1_name});
  std::string output_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC2", {act_name}, hidden_size, output_size);

  ////////////////////////////////////////
  /// DEFINING DATA AND LABELS FOR XOR ///
  ////////////////////////////////////////

  TypeParam data{{4, 1}};
  data.Set(0, 0, DataType(1));
  data.Set(1, 0, DataType(2));
  data.Set(2, 0, DataType(3));
  data.Set(3, 0, DataType(4));

  TypeParam gt{{4, 1}};
  gt.Set(0, 0, DataType(2));
  gt.Set(1, 0, DataType(3));
  gt.Set(2, 0, DataType(4));
  gt.Set(3, 0, DataType(5));

  //////////////////////
  /// Initialize SGD ///
  //////////////////////

  fetch::ml::AdaGradOptimizer<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>> optimizer(
      g, x_input_name, output_name, learning_rate);

  /////////////////////
  /// TRAINING LOOP ///
  /////////////////////

  DataType loss;
  for (std::size_t i = 0; i < n_batches; ++i)
  {
    loss = optimizer.DoBatch(data, gt);
  }
  EXPECT_NEAR(static_cast<double>(loss), 10.477067716419697, 1e-5);
}

TYPED_TEST(OptimizersTest, rmsprop_optimizer_training)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.01f};
  DataType decay_rate    = DataType{0.9f};
  SizeType input_size    = SizeType(1);
  SizeType output_size   = SizeType(1);
  SizeType n_batches     = SizeType(3);
  SizeType hidden_size   = SizeType(100);

  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      std::shared_ptr<fetch::ml::Graph<TypeParam>>(new fetch::ml::Graph<TypeParam>());

  std::string x_input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});

  std::string fc1_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC1", {x_input_name}, input_size, hidden_size);
  std::string act_name    = g->template AddNode<fetch::ml::ops::Relu<TypeParam>>("", {fc1_name});
  std::string output_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC2", {act_name}, hidden_size, output_size);

  ////////////////////////////////////////
  /// DEFINING DATA AND LABELS FOR XOR ///
  ////////////////////////////////////////

  TypeParam data{{4, 1}};
  data.Set(0, 0, DataType(1));
  data.Set(1, 0, DataType(2));
  data.Set(2, 0, DataType(3));
  data.Set(3, 0, DataType(4));

  TypeParam gt{{4, 1}};
  gt.Set(0, 0, DataType(2));
  gt.Set(1, 0, DataType(3));
  gt.Set(2, 0, DataType(4));
  gt.Set(3, 0, DataType(5));

  //////////////////////
  /// Initialize SGD ///
  //////////////////////

  fetch::ml::RMSPropOptimizer<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>> optimizer(
      g, x_input_name, output_name, learning_rate, decay_rate);

  /////////////////////
  /// TRAINING LOOP ///
  /////////////////////

  DataType loss;
  for (std::size_t i = 0; i < n_batches; ++i)
  {
    loss = optimizer.DoBatch(data, gt);
  }
  EXPECT_NEAR(static_cast<double>(loss), 2.0320102334953845, 1e-5);
}

TYPED_TEST(OptimizersTest, adam_optimizer_training)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  DataType learning_rate = DataType{0.01f};
  DataType beta1         = DataType{0.9f};
  DataType beta2         = DataType{0.999f};
  SizeType input_size    = SizeType(1);
  SizeType output_size   = SizeType(1);
  SizeType n_batches     = SizeType(3);
  SizeType hidden_size   = SizeType(100);

  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      std::shared_ptr<fetch::ml::Graph<TypeParam>>(new fetch::ml::Graph<TypeParam>());

  std::string x_input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});

  std::string fc1_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC1", {x_input_name}, input_size, hidden_size);
  std::string act_name    = g->template AddNode<fetch::ml::ops::Relu<TypeParam>>("", {fc1_name});
  std::string output_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC2", {act_name}, hidden_size, output_size);

  ////////////////////////////////////////
  /// DEFINING DATA AND LABELS FOR XOR ///
  ////////////////////////////////////////

  TypeParam data{{4, 1}};
  data.Set(0, 0, DataType(1));
  data.Set(1, 0, DataType(2));
  data.Set(2, 0, DataType(3));
  data.Set(3, 0, DataType(4));

  TypeParam gt{{4, 1}};
  gt.Set(0, 0, DataType(2));
  gt.Set(1, 0, DataType(3));
  gt.Set(2, 0, DataType(4));
  gt.Set(3, 0, DataType(5));

  //////////////////////
  /// Initialize SGD ///
  //////////////////////

  fetch::ml::AdamOptimizer<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>> optimizer(
      g, x_input_name, output_name, learning_rate, beta1, beta2);

  /////////////////////
  /// TRAINING LOOP ///
  /////////////////////

  DataType loss;
  for (std::size_t i = 0; i < n_batches; ++i)
  {
    loss = optimizer.DoBatch(data, gt);
  }
  EXPECT_NEAR(static_cast<double>(loss), 10.252499443013221, 1e-5);
}
