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
#include "ml/ops/loss_functions.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/optimisation/lazy_adam_optimiser.hpp"
#include "ml/saveparams/saveable_params.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class SparseOptimisersTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SparseOptimisersTest, math::test::HighPrecisionTensorFloatingTypes);

//////////////////////////
/// reusable functions ///
//////////////////////////
namespace sparse_optimiser_details {

template <typename TypeParam>
std::shared_ptr<fetch::ml::Graph<TypeParam>> PrepareTestGraph(
    typename TypeParam::SizeType embedding_dimensions, typename TypeParam::SizeType n_datapoints,
    std::string &input_name, std::string &label_name, std::string &error_name)
{
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g(std::make_shared<fetch::ml::Graph<TypeParam>>());

  input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});

  std::string output_name = g->template AddNode<fetch::ml::ops::Embeddings<TypeParam>>(
      "Embeddings", {input_name}, embedding_dimensions, n_datapoints);

  label_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});
  error_name = g->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "Error", {output_name, label_name});

  return g;
}

template <typename TypeParam>
void PrepareTestDataAndLabelsFirst(TypeParam &data, TypeParam &gt)
{
  using DataType = typename TypeParam::Type;

  data.Resize({1, 4});
  data.Set(0, 0, static_cast<DataType>(4));
  data.Set(0, 1, DataType(8));
  data.Set(0, 2, DataType(9));
  data.Set(0, 3, DataType(15));

  gt.Resize({10, 1, 4});
  gt.Set(2, 0, 0, DataType(-10));
  gt.Set(3, 0, 1, DataType(10));
  gt.Set(4, 0, 2, DataType(-5));
  gt.Set(5, 0, 3, DataType(5));
}

template <typename TypeParam>
void PrepareTestDataAndLabelsSecond(TypeParam &data, TypeParam &gt)
{
  using DataType = typename TypeParam::Type;

  data.Resize({1, 4});
  data.Set(0, 0, static_cast<DataType>(5));
  data.Set(0, 1, DataType(8));
  data.Set(0, 2, DataType(10));
  data.Set(0, 3, DataType(15));

  gt.Resize({10, 1, 4});
  gt.Set(2, 0, 0, DataType(-10));
  gt.Set(3, 0, 1, DataType(10));
  gt.Set(4, 0, 2, DataType(-5));
  gt.Set(5, 0, 3, DataType(5));
}

}  // namespace sparse_optimiser_details

TYPED_TEST(SparseOptimisersTest, lazy_adam_optimiser_training_2D)
{
  // On LazyAdam only currently changes values are updated with momentum and moving square average
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.01f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      sparse_optimiser_details::PrepareTestGraph<TypeParam>(10, 50, input_name, label_name,
                                                            output_name);

  // Prepare data and labels
  TypeParam data_1;
  TypeParam gt_1;
  sparse_optimiser_details::PrepareTestDataAndLabelsFirst(data_1, gt_1);

  TypeParam data_2;
  TypeParam gt_2;
  sparse_optimiser_details::PrepareTestDataAndLabelsSecond(data_2, gt_2);

  // Initialise Optimiser
  fetch::ml::optimisers::LazyAdamOptimiser<TypeParam> optimiser(g, {input_name}, label_name,
                                                                output_name, learning_rate);

  // Do multiple steps
  DataType loss1 = optimiser.Run({data_1}, gt_1);
  optimiser.Run({data_2}, gt_2);

  // Test weights
  std::vector<TypeParam> weights = g->GetWeights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(7, 0)), -0.15218086540699005,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 2 *
                  static_cast<double>(gt_1.size()));
  EXPECT_NEAR(static_cast<double>(weights[0].At(4, 4)), 0.17170494794845581,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 2 *
                  static_cast<double>(gt_1.size()));
  EXPECT_NEAR(static_cast<double>(weights[0].At(8, 32)), 0.094521790742874146,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 2 *
                  static_cast<double>(gt_1.size()));
  EXPECT_NEAR(static_cast<double>(weights[0].At(0, 9)), -0.022590959910303354,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 2 *
                  static_cast<double>(gt_1.size()));

  for (math::SizeType i{0}; i < 50; i++)
  {
    optimiser.Run({data_2}, gt_2);
  }
  DataType loss2 = optimiser.Run({data_2}, gt_2);

  // Test loss
  EXPECT_LE(static_cast<double>(loss2), static_cast<double>(loss1));
}

TYPED_TEST(SparseOptimisersTest, adam_optimiser_training_2D)
{
  // On normal adam all values are updated with momentum and moving square average
  using DataType = typename TypeParam::Type;

  auto learning_rate = DataType{0.01f};

  // Prepare model
  std::string                                  input_name;
  std::string                                  label_name;
  std::string                                  output_name;
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
      sparse_optimiser_details::PrepareTestGraph<TypeParam>(10, 50, input_name, label_name,
                                                            output_name);

  // Prepare data and labels
  TypeParam data_1;
  TypeParam gt_1;
  sparse_optimiser_details::PrepareTestDataAndLabelsFirst(data_1, gt_1);

  TypeParam data_2;
  TypeParam gt_2;
  sparse_optimiser_details::PrepareTestDataAndLabelsSecond(data_2, gt_2);

  // Initialise Optimiser
  fetch::ml::optimisers::AdamOptimiser<TypeParam> optimiser(g, {input_name}, label_name,
                                                            output_name, learning_rate);

  // Do multiple steps
  DataType loss1 = optimiser.Run({data_1}, gt_1);
  optimiser.Run({data_2}, gt_2);

  // Test weights
  std::vector<TypeParam> weights = g->GetWeights();
  EXPECT_NEAR(static_cast<double>(weights[0].At(7, 0)), -0.15218086540699005,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 2 *
                  static_cast<double>(gt_1.size()));
  EXPECT_NEAR(static_cast<double>(weights[0].At(4, 4)), 0.16570879518985748,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 2 *
                  static_cast<double>(gt_1.size()));
  EXPECT_NEAR(static_cast<double>(weights[0].At(8, 32)), 0.094521790742874146,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 2 *
                  static_cast<double>(gt_1.size()));
  EXPECT_NEAR(static_cast<double>(weights[0].At(0, 9)), -0.01671302760951221,
              static_cast<double>(fetch::math::function_tolerance<DataType>()) * 2 *
                  static_cast<double>(gt_1.size()));

  for (math::SizeType i{0}; i < 50; i++)
  {
    optimiser.Run({data_2}, gt_2);
  }
  DataType loss2 = optimiser.Run({data_2}, gt_2);

  // Test loss
  EXPECT_LE(static_cast<double>(loss2), static_cast<double>(loss1));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
