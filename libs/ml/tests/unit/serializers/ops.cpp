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

#include "core/serializers/main_serializer.hpp"
#include "gtest/gtest.h"
#include "math/base_types.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "test_types.hpp"

#include <memory>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class SaveParamsTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SaveParamsTest, math::test::TensorFloatingTypes);


/////////////////////////
/// REDUCE MEAN TESTS ///
/////////////////////////

TYPED_TEST(SaveParamsTest, reduce_mean_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::ReduceMean<TensorType>::SPType;
  using OpType        = fetch::ml::ops::ReduceMean<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});

  OpType op(1);

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
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SaveParamsTest, reduce_mean_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::ReduceMean<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});
  TensorType error = TensorType::FromString("1, -2, -1, 2");
  error.Reshape({2, 1, 2});

  fetch::ml::ops::ReduceMean<TypeParam> op(1);

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

TYPED_TEST(SaveParamsTest, ReduceMean_graph_serialization_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = fetch::ml::GraphSaveableParams<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});

  fetch::ml::Graph<TensorType> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string output_name =
      g.template AddNode<fetch::ml::ops::ReduceMean<TensorType>>("Output", {input_name}, 1);

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

/////////////////////
/// RESHAPE TESTS ///
/////////////////////


TYPED_TEST(SaveParamsTest, Reshape_graph_serialisation_test)
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


TYPED_TEST(SaveParamsTest, slice_single_axis_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Slice<TensorType>::SPType;
  using SizeVector    = typename TypeParam::SizeVector;

  TypeParam  data({1, 2, 3, 4, 5});
  SizeVector axes({3});
  SizeVector indices({3});

  fetch::ml::ops::Slice<TypeParam> op(indices, axes);

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
  fetch::ml::ops::Slice<TensorType> new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, slice_single_axis_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::Slice<TensorType>;
  using SPType     = typename OpType::SPType;
  using SizeType   = fetch::math::SizeType;

  TypeParam data = TypeParam::FromString("1, 1, 3, 141; 4, 52, 6, 72; -1, -2, -19, -4");
  data.Reshape({3, 2, 2});
  SizeType axis(1u);
  SizeType index(0u);

  TypeParam error = TypeParam::FromString("1, 3; 4, 6; -1, -3");
  error.Reshape({3, 1, 2});

  fetch::ml::ops::Slice<TypeParam>               op(index, axis);
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

TYPED_TEST(SaveParamsTest, slice_ranged_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Slice<TensorType>::SPType;
  using SizeType      = fetch::math::SizeType;
  using SizePairType  = std::pair<SizeType, SizeType>;

  TypeParam data = TypeParam::FromString("1, 2, 3, 4; 4, 5, 6, 7; -1, -2, -3, -4");
  data.Reshape({3, 2, 2});

  SizeType     axis{0};
  SizePairType start_end_slice = std::make_pair(1, 3);

  fetch::ml::ops::Slice<TypeParam> op(start_end_slice, axis);

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
  fetch::ml::ops::Slice<TensorType> new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, slice_ranged_saveparams_backward_test)
{
  using TensorType   = TypeParam;
  using DataType     = typename TypeParam::Type;
  using OpType       = fetch::ml::ops::Slice<TensorType>;
  using SPType       = typename OpType::SPType;
  using SizeType     = fetch::math::SizeType;
  using SizePairType = std::pair<SizeType, SizeType>;

  TypeParam data = TypeParam::FromString("1, 1, 3, 141; 4, 52, 6, 72; -1, -2, -19, -4");
  data.Reshape({3, 2, 2});

  SizeType     axis{0};
  SizePairType start_end_slice = std::make_pair(1, 3);

  TypeParam error = TypeParam::FromString("1, 3; 4, 6; -1, -3; -2, -3");
  error.Reshape({2, 2, 2});

  fetch::ml::ops::Slice<TypeParam>               op(start_end_slice, axis);
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

///////////////////
/// SLICE TESTS ///
///////////////////

TYPED_TEST(SaveParamsTest, slice_multi_axes_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Slice<TensorType>::SPType;
  using SizeVector    = typename TypeParam::SizeVector;

  TypeParam data = TypeParam::FromString("1, 2, 3, 4; 4, 5, 6, 7; -1, -2, -3, -4");
  data.Reshape({3, 2, 2});
  SizeVector axes({1, 2});
  SizeVector indices({1, 1});

  fetch::ml::ops::Slice<TypeParam> op(indices, axes);

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
  fetch::ml::ops::Slice<TensorType> new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

/////////////////////
/// SQUEEZE TESTS ///
/////////////////////

TYPED_TEST(SaveParamsTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Squeeze<TensorType>::SPType;
  using OpType        = fetch::ml::ops::Squeeze<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");
  data.Reshape({6, 1});

  OpType op;

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
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SaveParamsTest, saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::Squeeze<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TensorType::FromString("1, -2, 4, -10, 100");
  data.Reshape({1, 5});
  TensorType error = TensorType::FromString("1, 1, 1, 2, 0");
  error.Reshape({5});

  fetch::ml::ops::Squeeze<TypeParam> op;

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

TYPED_TEST(SaveParamsTest, squeeze_graph_serialization_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = fetch::ml::GraphSaveableParams<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");
  data.Reshape({6, 1});

  fetch::ml::Graph<TensorType> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string output_name =
      g.template AddNode<fetch::ml::ops::Squeeze<TensorType>>("Output", {input_name});

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

}  // namespace test
}  // namespace ml
}  // namespace fetch
