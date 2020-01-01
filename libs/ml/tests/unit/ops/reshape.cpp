//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/serializers/main_serializer_definition.hpp"
#include "math/base_types.hpp"
#include "ml/core/graph.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/reshape.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

using SizeType = fetch::math::SizeType;

template <typename T>
class ReshapeTest : public ::testing::Test
{
};

TYPED_TEST_CASE(ReshapeTest, fetch::math::test::TensorFloatingTypes);

template <typename TensorType>
void ReshapeTestForward(std::vector<SizeType> const &initial_shape,
                        std::vector<SizeType> const &final_shape)
{
  TensorType                          data(initial_shape);
  TensorType                          gt(final_shape);
  fetch::ml::ops::Reshape<TensorType> op(final_shape);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  EXPECT_EQ(prediction.shape().size(), final_shape.size());
  for (std::size_t j = 0; j < final_shape.size(); ++j)
  {
    EXPECT_EQ(prediction.shape().at(j), final_shape.at(j));
  }
}

template <typename TensorType>
void ReshapeTestForwardWrong(std::vector<SizeType> const &initial_shape,
                             std::vector<SizeType> const &final_shape)
{
  TensorType                          data(initial_shape);
  TensorType                          gt(final_shape);
  fetch::ml::ops::Reshape<TensorType> op(final_shape);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  EXPECT_ANY_THROW(op.Forward({std::make_shared<const TensorType>(data)}, prediction));
}

template <typename TensorType>
void ReshapeTestBackward(std::vector<SizeType> const &initial_shape,
                         std::vector<SizeType> const &final_shape)
{

  TensorType data(initial_shape);
  TensorType error(final_shape);
  TensorType gt_error(initial_shape);

  data.FillUniformRandom();
  gt_error = data.Copy();

  // call reshape and store result in 'error'
  fetch::ml::ops::Reshape<TensorType>            op(final_shape);
  std::vector<std::shared_ptr<const TensorType>> data_vec({std::make_shared<TensorType>(data)});
  op.Forward(data_vec, error);

  // backprop with incoming error signal matching output data
  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  for (std::size_t j = 0; j < error_signal.at(0).shape().size(); ++j)
  {
    EXPECT_EQ(error_signal.at(0).shape().at(j), gt_error.shape().at(j));
  }

  EXPECT_TRUE(error_signal.at(0).AllClose(gt_error));
}

TYPED_TEST(ReshapeTest, forward_tests)
{
  ReshapeTestForward<TypeParam>({3, 2, 1}, {6, 1, 1});
  ReshapeTestForward<TypeParam>({6, 1, 2}, {3, 2, 2});
  ReshapeTestForward<TypeParam>({6, 1, 3}, {6, 1, 3});
  ReshapeTestForward<TypeParam>({6, 1, 4}, {6, 1, 1, 4});
  ReshapeTestForward<TypeParam>({3, 2, 5}, {6, 1, 1, 1, 5});

  ReshapeTestForward<TypeParam>({3, 2, 1}, {6, 1, 1});
  ReshapeTestForward<TypeParam>({6, 1, 1, 2}, {6, 1, 2});
  ReshapeTestForward<TypeParam>({6, 1, 1, 1, 3}, {3, 2, 3});

  ReshapeTestForward<TypeParam>({7, 6, 5, 4, 3, 2, 1, 1}, {7, 6, 5, 4, 3, 2, 1});
  ReshapeTestForward<TypeParam>({1, 2, 3, 4, 5, 6, 7, 2}, {7, 6, 5, 4, 3, 2, 1, 2});
  ReshapeTestForward<TypeParam>({1, 2, 3, 4, 5, 6, 7, 3}, {5040, 1, 1, 1, 1, 3});
}

TYPED_TEST(ReshapeTest, forward_wrong_tests)
{
  ReshapeTestForwardWrong<TypeParam>({3, 4}, {6, 1});
  ReshapeTestForwardWrong<TypeParam>({6, 2, 1}, {6, 1});
  ReshapeTestForwardWrong<TypeParam>({7, 6, 5, 4, 3, 2, 1}, {7, 6, 5, 1});
  ReshapeTestForwardWrong<TypeParam>({3, 4, 1}, {6, 1, 1});
  ReshapeTestForwardWrong<TypeParam>({6, 1, 2, 1}, {6, 1, 1});
  ReshapeTestForwardWrong<TypeParam>({7, 6, 5, 4, 3, 2, 1, 1}, {7, 6, 5, 1});
}

TYPED_TEST(ReshapeTest, backward_tests)
{
  ReshapeTestBackward<TypeParam>({3, 2, 5}, {6, 1, 5});
  ReshapeTestBackward<TypeParam>({6, 1, 6}, {3, 2, 6});
  ReshapeTestBackward<TypeParam>({6, 1, 7}, {6, 1, 7});
  ReshapeTestBackward<TypeParam>({6, 1, 8}, {6, 1, 1, 8});
  ReshapeTestBackward<TypeParam>({3, 2, 9}, {6, 1, 1, 1, 9});

  ReshapeTestBackward<TypeParam>({3, 2, 2}, {6, 1, 2});
  ReshapeTestBackward<TypeParam>({6, 1, 1, 3}, {6, 1, 3});
  ReshapeTestBackward<TypeParam>({6, 1, 1, 1, 4}, {3, 2, 4});

  ReshapeTestBackward<TypeParam>({7, 6, 5, 4, 3, 2, 1, 7}, {7, 6, 5, 4, 3, 2, 7});
  ReshapeTestBackward<TypeParam>({1, 2, 3, 4, 5, 6, 7, 3}, {7, 6, 5, 4, 3, 2, 1, 3});
  ReshapeTestBackward<TypeParam>({1, 2, 3, 4, 5, 6, 7, 1}, {5040, 1, 1, 1, 1, 1});
}

TYPED_TEST(ReshapeTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Reshape<TensorType>::SPType;

  // construct tensor & reshape op
  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2, 1});
  fetch::ml::ops::Reshape<TensorType> op({8, 1, 1, 1});

  // forward pass
  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});
  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  fetch::ml::ops::Reshape<TensorType> new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(ReshapeTest, saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::Reshape<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2, 1});
  TensorType error = TensorType::FromString("1, -2, -1, 2");
  error.Reshape({8, 1, 1});

  // call reshape and store result in 'error'
  fetch::ml::ops::Reshape<TypeParam>             op({8, 1, 1});
  std::vector<std::shared_ptr<const TensorType>> data_vec({std::make_shared<TensorType>(data)});
  op.Forward(data_vec, error);

  // backprop with incoming error signal matching output data
  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // get another error_signal with the original op
  error_signal = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(ReshapeTest, Reshape_graph_serialisation_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = fetch::ml::GraphSaveableParams<TensorType>;

  std::vector<SizeType> final_shape({8, 1, 1, 1});

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2, 1});

  fetch::ml::Graph<TensorType> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string output_name =
      g.template AddNode<fetch::ml::ops::Reshape<TensorType>>("Output", {input_name}, final_shape);

  g.SetInput(input_name, data);
  TypeParam output = g.Evaluate("Output");

  // extract saveparams
  SPType gsp = g.GetGraphSaveableParams();

  fetch::serializers::MsgPackSerializer b;
  b << gsp;

  // deserialize
  b.seek(0);
  SPType dsp2;
  b >> dsp2;

  // rebuild graph
  auto new_graph_ptr = std::make_shared<fetch::ml::Graph<TensorType>>();
  fetch::ml::utilities::BuildGraph(gsp, new_graph_ptr);

  new_graph_ptr->SetInput(input_name, data);
  TypeParam output2 = new_graph_ptr->Evaluate("Output");

  // Test correct values
  ASSERT_EQ(output.shape(), output2.shape());
  ASSERT_TRUE(output.AllClose(output2, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));
}

}  // namespace
