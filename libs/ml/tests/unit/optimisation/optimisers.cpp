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

#include "gtest/gtest.h"
#include "ml/core/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/loss_functions.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/optimisation/adagrad_optimiser.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/optimisation/momentum_optimiser.hpp"
#include "ml/optimisation/rmsprop_optimiser.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"
#include "ml/saveparams/saveable_params.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class OptimisersTest : public ::testing::Test
{
};

TYPED_TEST_CASE(OptimisersTest, math::test::HighPrecisionTensorFloatingTypes);

//////////////////////////
/// reusable functions ///
//////////////////////////

template <typename TypeParam>
std::shared_ptr<fetch::ml::Graph<TypeParam>> PrepareTestGraph(
    typename TypeParam::SizeType input_size, typename TypeParam::SizeType output_size,
    std::string &input_name, std::string &label_name, std::string &error_name)
{
  using SizeType = fetch::math::SizeType;

  auto hidden_size = SizeType(10);

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

/////////////////
/// SGD TESTS ///
/////////////////

TYPED_TEST(OptimisersTest, sgd_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.001f};

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
  DataType loss1 = optimiser.Run({data}, gt);
  DataType loss2 = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_LE(static_cast<double>(loss2), static_cast<double>(loss1));

  // Test weights
  std::vector<TypeParam> weights = g->GetWeights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.00059174111811444163,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.37465476989746094,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.0018975307466462255,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.30353739857673645,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, sgd_optimiser_training_2D)
{
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.0001f};

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
  DataType loss1 = optimiser.Run({data}, gt);
  DataType loss2 = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_LE(static_cast<double>(loss2), static_cast<double>(loss1));

  // Test weights
  std::vector<TypeParam> weights = g->GetWeights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.33262497186660767,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.00040797272231429815,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.3599897027015686,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, sgd_optimiser_serialisation)
{
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.06f};

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

  // serialise the optimiser
  fetch::serializers::MsgPackSerializer b;
  b << optimiser;

  // deserialis the optimiser
  b.seek(0);
  auto optimiser_2 = std::make_shared<fetch::ml::optimisers::SGDOptimiser<TypeParam>>();
  b >> *optimiser_2;

  // Do 2 optimiser steps
  loss                  = optimiser.Run({data}, gt);
  DataType const loss_2 = optimiser_2->Run({data}, gt);

  // Test loss
  EXPECT_EQ(loss, loss_2);
}

//////////////////////
/// MOMENTUM TESTS ///
//////////////////////

TYPED_TEST(OptimisersTest, momentum_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.16f};

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
  DataType loss1 = optimiser.Run({data}, gt);
  DataType loss2 = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_LE(static_cast<double>(loss2), static_cast<double>(loss1));

  // Test weights
  std::vector<TypeParam> weights = g->GetWeights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.090210896972293675,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.65327156457671065,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.28951357414687784,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.52928183723112876,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, momentum_optimiser_training_2D)
{
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.001f};

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
  DataType loss1 = optimiser.Run({data}, gt);
  DataType loss2 = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_LE(static_cast<double>(loss2), static_cast<double>(loss1));

  // Test weights
  std::vector<TypeParam> weights = g->GetWeights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.36166410636801072,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.0055758205512807472,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.3998452632639875,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, adagrad_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.04f};

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
  DataType loss1 = optimiser.Run({data}, gt);
  DataType loss2 = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_LE(static_cast<double>(loss2), static_cast<double>(loss1));

  // Test weights
  std::vector<TypeParam> weights = g->GetWeights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.065168781871604062,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.43727626598809755,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.063317083714749728,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.36751842419922826,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, adagrad_optimiser_training_2D)
{
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.04f};

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
  DataType loss1 = optimiser.Run({data}, gt);
  DataType loss2 = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_LE(static_cast<double>(loss2), static_cast<double>(loss1));

  // Test weights
  std::vector<TypeParam> weights = g->GetWeights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.3902450374003274,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.051480459546604504,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.40842423372627268,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, rmsprop_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.01f};

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
  DataType loss1 = optimiser.Run({data}, gt);
  DataType loss2 = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_LE(static_cast<double>(loss2), static_cast<double>(loss1));

  // Test weights
  std::vector<TypeParam> weights = g->GetWeights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.052843646006995849,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.4252042899869915,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.051698915451037164,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.35507014458490305,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, rmsprop_optimiser_training_2D)
{
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.01f};

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
  DataType loss1 = optimiser.Run({data}, gt);
  DataType loss2 = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_LE(static_cast<double>(loss2), static_cast<double>(loss1));

  // Test weights
  std::vector<TypeParam> weights = g->GetWeights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.38034263943110036,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.045177050585783071,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.40197003220162975,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, adam_optimiser_training)
{
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.01f};

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
  DataType loss1 = optimiser.Run({data}, gt);
  DataType loss2 = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_LE(static_cast<double>(loss2), static_cast<double>(loss1));

  // Test weights
  std::vector<TypeParam> weights = g->GetWeights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.021624941488699607,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.3943416952903428,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.021612225474637425,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.32360038170687122,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, adam_optimiser_training_2D)
{
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.01f};

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
  DataType loss1 = optimiser.Run({data}, gt);
  DataType loss2 = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_LE(static_cast<double>(loss2), static_cast<double>(loss1));

  // Test weights
  std::vector<TypeParam> weights = g->GetWeights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.35197555714182616,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.021518885957721133,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.37836752039704996,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

TYPED_TEST(OptimisersTest, adam_optimiser_minibatch_training)
{
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.01f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =

      PrepareTestGraph<TypeParam>(1u, 1u, input_name, label_name, output_name);

  // Prepare data and labels
  TypeParam data;
  TypeParam gt;
  PrepareTestDataAndLabels1D(data, gt);

  // Initialise Optimiser
  fetch::ml::optimisers::AdamOptimiser<TypeParam> optimiser(g, {input_name}, label_name,
                                                            output_name, learning_rate);

  // Do multiple steps
  DataType loss1 = optimiser.Run({data}, gt);
  DataType loss2 = optimiser.Run({data}, gt);

  // Test loss
  EXPECT_LE(static_cast<double>(loss2), static_cast<double>(loss1));

  // Test weights
  std::vector<TypeParam> weights = g->GetWeights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.021624941488699607,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.3943416952903428,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.021612225474637425,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.32360038170687122,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
