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
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.00037039007293060422,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.24058516323566437,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.0024778733495622873,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.33308404684066772,
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
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.00036449235631152987,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.21178488433361053,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.00055545743089169264,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.22925795614719391,
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
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.06953313946723938,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.54472243785858154,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.43430191278457642,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.7578970193862915,
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
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.0051308656111359596,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.21178488433361053,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.0078205317258834839,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.23670780658721924,
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
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.068447768688201904,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.30608183145523071,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.064945906400680542,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.39790481328964233,
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
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.061900380998849869,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.21178488433361053,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.063398018479347229,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.30402284860610962,
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
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.054797373712062836,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.29292047023773193,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.052629902958869934,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.38475692272186279,
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
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.051219813525676727,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.21178488433361053,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.051959723234176636,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.28830838203430176,
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
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.021644443273544312,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.26056835055351257,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.021624732296913862,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.35242897272109985,
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
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.021613573655486107,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.21178488433361053,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.021623954176902771,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.25041872262954712,
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
  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.021644443273544312,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), 0.26056835055351257,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.021624732296913862,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), 0.35242897272109985,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) *
                  static_cast<double>(data.size()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
