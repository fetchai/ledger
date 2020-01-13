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

///////////////////////
/// MATRIX MULTIPLY ///
///////////////////////

TYPED_TEST(SaveParamsTest, matrix_multiply_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MatrixMultiply<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::MatrixMultiply<TensorType>;

  TypeParam data_1 = TypeParam::FromString("1, 2, -3, 4, 5");
  TypeParam data_2 = TypeParam::FromString(
      "-11, 12, 13, 14; 21, 22, 23, 24; 31, 32, 33, 34; 41, 42, 43, 44; 51, 52, 53, 54");

  OpType op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

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
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, matrix_multiply_saveparams_backward_batch_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::MatrixMultiply<TensorType>;
  using SPType     = typename OpType::SPType;
  TypeParam a1({3, 4, 2});
  TypeParam b1({4, 3, 2});
  TypeParam error({3, 3, 2});
  TypeParam gradient_a({3, 4, 2});
  TypeParam gradient_b({4, 3, 2});

  fetch::ml::ops::MatrixMultiply<TypeParam> op;
  std::vector<TypeParam>                    backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a1), std::make_shared<TypeParam>(b1)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a1), std::make_shared<TypeParam>(b1)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TypeParam> new_backpropagated_signals =
      new_op.Backward({std::make_shared<TypeParam>(a1), std::make_shared<TypeParam>(b1)}, error);

  // test correct values
  EXPECT_TRUE(backpropagated_signals.at(0).AllClose(
      new_backpropagated_signals.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));

  EXPECT_TRUE(backpropagated_signals.at(1).AllClose(
      new_backpropagated_signals.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////////////
/// MAX POOL ///
///////////////////

TYPED_TEST(SaveParamsTest, maxpool_saveparams_test_1d)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MaxPool<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::MaxPool<TensorType>;
  using SizeType      = typename TypeParam::SizeType;

  TensorType          data({2, 5, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gt_input({3, 5, 9, 9});

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 5; ++j)
      {
        data(i, j, i_b) = fetch::math::AsType<DataType>(data_input[i * 5 + j]) +
                          fetch::math::AsType<DataType>(i_b * 10);
      }
    }

    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 2; ++j)
      {
        gt(i, j, i_b) = fetch::math::AsType<DataType>(gt_input[i * 2 + j]) +
                        fetch::math::AsType<DataType>(i_b * 10);
      }
    }
  }

  OpType op(4, 1);

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
}

TYPED_TEST(SaveParamsTest, maxpool_saveparams_backward_test_1d_2_channels)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;
  using OpType     = typename fetch::ml::ops::MaxPool<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType          data({2, 5, 2});
  TensorType          error({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 10, -6, 7, -8, 9, -10});
  std::vector<double> errorInput({2, 3, 4, 5});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 5; ++j)
    {
      data(i, j, 0) = fetch::math::AsType<DataType>(data_input[i * 5 + j]);
    }
  }

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      error(i, j, 0) = fetch::math::AsType<DataType>(errorInput[i * 2 + j]);
    }
  }

  fetch::ml::ops::MaxPool<TensorType> op(4, 1);
  std::vector<TensorType>             prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(SaveParamsTest, maxpool_saveparams_test_2d)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MaxPool2D<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::MaxPool2D<TensorType>;
  using SizeType      = typename TypeParam::SizeType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 10;
  SizeType const input_height  = 5;

  SizeType const batch_size = 2;

  TensorType data({channels_size, input_width, input_height, batch_size});

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < input_width; ++i)
    {
      for (SizeType j{0}; j < input_height; ++j)
      {
        data(c, i, j, 0) = fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  OpType op(3, 2);

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
}

TYPED_TEST(SaveParamsTest, maxpool_saveparams_backward_2_channels_test_2d)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;
  using OpType     = typename fetch::ml::ops::MaxPool2D<TensorType>;
  using SPType     = typename OpType::SPType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 5;
  SizeType const input_height  = 5;
  SizeType const output_width  = 2;
  SizeType const output_height = 2;
  SizeType const batch_size    = 2;

  TensorType data({channels_size, input_width, input_height, batch_size});
  TensorType error({channels_size, output_width, output_height, batch_size});

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < input_width; ++i)
    {
      for (SizeType j{0}; j < input_height; ++j)
      {
        data(c, i, j, 0) = fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < output_width; ++i)
    {
      for (SizeType j{0}; j < output_height; ++j)
      {
        error(c, i, j, 0) = fetch::math::AsType<DataType>((c + 1) * (1 + i + j));
      }
    }
  }

  fetch::ml::ops::MaxPool2D<TensorType> op(3, 2);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////////////
/// MAX POOL 1D ///
///////////////////

TYPED_TEST(SaveParamsTest, maxpool_1d_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MaxPool1D<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::MaxPool1D<TensorType>;
  using SizeType      = fetch::math::SizeType;

  TensorType          data({2, 5, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 5; ++j)
      {
        data(i, j, i_b) = fetch::math::AsType<DataType>(data_input[i * 5 + j]) +
                          fetch::math::AsType<DataType>(i_b * 10);
      }
    }
  }

  OpType op(4, 1);

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
}

TYPED_TEST(SaveParamsTest, maxpool_1d_saveparams_backward_test_2_channels)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;
  using OpType     = typename fetch::ml::ops::MaxPool1D<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType          data({2, 5, 2});
  TensorType          error({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 10, -6, 7, -8, 9, -10});
  std::vector<double> errorInput({2, 3, 4, 5});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 5; ++j)
    {
      data(i, j, 0) = fetch::math::AsType<DataType>(data_input[i * 5 + j]);
    }
  }

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      error(i, j, 0) = fetch::math::AsType<DataType>(errorInput[i * 2 + j]);
    }
  }

  fetch::ml::ops::MaxPool1D<TensorType> op(4, 1);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////////////
/// MAX POOL 2D ///
///////////////////

TYPED_TEST(SaveParamsTest, maxpool_2d_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MaxPool2D<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::MaxPool2D<TensorType>;
  using SizeType      = fetch::math::SizeType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 10;
  SizeType const input_height  = 5;

  SizeType const batch_size = 2;

  TensorType data({channels_size, input_width, input_height, batch_size});

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < input_width; ++i)
    {
      for (SizeType j{0}; j < input_height; ++j)
      {
        data(c, i, j, 0) = fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  OpType op(3, 2);

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
}

TYPED_TEST(SaveParamsTest, maxpool_2d_saveparams_backward_2_channels_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;
  using OpType     = typename fetch::ml::ops::MaxPool2D<TensorType>;
  using SPType     = typename OpType::SPType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 5;
  SizeType const input_height  = 5;
  SizeType const output_width  = 2;
  SizeType const output_height = 2;
  SizeType const batch_size    = 2;

  TensorType data({channels_size, input_width, input_height, batch_size});
  TensorType error({channels_size, output_width, output_height, batch_size});

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < input_width; ++i)
    {
      for (SizeType j{0}; j < input_height; ++j)
      {
        data(c, i, j, 0) = fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < output_width; ++i)
    {
      for (SizeType j{0}; j < output_height; ++j)
      {
        error(c, i, j, 0) = fetch::math::AsType<DataType>((c + 1) * (1 + i + j));
      }
    }
  }

  fetch::ml::ops::MaxPool2D<TensorType> op(3, 2);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////////
/// MAXIMUM ///
///////////////

TYPED_TEST(SaveParamsTest, maximum_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Maximum<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::Maximum<TensorType>;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  OpType op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

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
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, maximum_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::Maximum<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Maximum<TensorType> op;
  std::vector<TensorType>             prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////////
/// MULTIPLY ///
////////////////

TYPED_TEST(SaveParamsTest, multiply_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Multiply<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::Multiply<TensorType>;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  OpType op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

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
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, multiply_saveparams_backward_test_NB_NB)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::Multiply<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Multiply<TensorType> op;
  std::vector<TensorType>              prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));

  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));

  ASSERT_TRUE(!fetch::math::state_overflow<typename TypeParam::Type>());
}

///////////////
/// ONE-HOT ///
///////////////

TYPED_TEST(SaveParamsTest, one_hot_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using SizeType      = typename TypeParam::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::OneHot<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::OneHot<TensorType>;

  TensorType data = TypeParam::FromString("1,0,1,2");
  data.Reshape({2, 2, 1, 1});

  SizeType depth     = 3;
  SizeType axis      = 3;
  auto     on_value  = DataType{5};
  auto     off_value = DataType{-1};

  OpType op(depth, axis, on_value, off_value);

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
}

///////////////////
/// PLACEHOLDER ///
///////////////////

TYPED_TEST(SaveParamsTest, placeholder_saveable_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::PlaceHolder<TensorType>::SPType;
  using OpType     = typename fetch::ml::ops::PlaceHolder<TensorType>;

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  OpType op;
  op.SetData(data);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));

  op.Forward({}, prediction);

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

  // placeholders do not store their data in serialisation, so we re set the data here
  new_op.SetData(data);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward({}, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

////////////////
/// PRELU_OP ///
////////////////

TYPED_TEST(SaveParamsTest, prelu_op_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::PReluOp<TensorType>::SPType;
  using OpType        = fetch::ml::ops::PReluOp<TensorType>;

  TensorType data =
      TensorType::FromString("1, -2, 3,-4, 5,-6, 7,-8; -1,  2,-3, 4,-5, 6,-7, 8").Transpose();

  TensorType alpha = TensorType::FromString("0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8").Transpose();

  OpType op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}));
  VecTensorType vec_data({std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)});

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
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, prelu_op_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpType     = fetch::ml::ops::PReluOp<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType alpha =
      TensorType::FromString(R"(0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8)").Transpose();

  TensorType data = TensorType::FromString(R"(1, -2, 3,-4, 5,-6, 7,-8;
                                             -1,  2,-3, 4,-5, 6,-7, 8)")
                        .Transpose();

  TensorType error = TensorType::FromString(R"(0, 0, 0, 0, 1, 1, 0, 0;
                                               0, 0, 0, 0, 1, 1, 0, 0)")
                         .Transpose();

  fetch::ml::ops::PReluOp<TensorType> op;
  std::vector<TensorType>             prediction =
      op.Backward({std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction =
      op.Backward({std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

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

  std::vector<math::SizeType> final_shape({8, 1, 1, 1});

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

TYPED_TEST(SaveParamsTest, reshape_saveparams_test)
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

TYPED_TEST(SaveParamsTest, reshape_saveparams_backward_test)
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

/////////////
/// SLICE ///
/////////////

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

////////////
/// SQRT ///
////////////

TYPED_TEST(SaveParamsTest, sqrt_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Sqrt<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::Sqrt<TensorType>;

  TensorType data = TensorType::FromString("0, 1, 2, 4, 10, 100");

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
}

TYPED_TEST(SaveParamsTest, sqrt_saveparams_backward_all_positive_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::Sqrt<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data  = TensorType::FromString("1,   2,         4,   10,       100");
  TensorType error = TensorType::FromString("1,   1,         1,    2,         0");

  fetch::ml::ops::Sqrt<TypeParam> op;

  // run op once to make sure caches etc. have been filled. Otherwise the test might be trivial!
  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
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

/////////////////////
/// STRIDED SLICE ///
/////////////////////

TYPED_TEST(SaveParamsTest, strided_slice_saveparams_test)
{
  using TensorType    = TypeParam;
  using SizeType      = fetch::math::SizeType;
  using SizeVector    = typename TypeParam::SizeVector;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::StridedSlice<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::StridedSlice<TensorType>;

  TypeParam input({9, 9, 9, 6, 4});
  TypeParam gt({6, 4, 3, 1, 2});

  SizeVector begins({3, 1, 0, 4, 0});
  SizeVector ends({8, 7, 8, 5, 2});
  SizeVector strides({1, 2, 3, 4, 2});

  auto     it  = input.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < gt.shape().at(0); i++)
  {
    for (SizeType j{0}; j < gt.shape().at(1); j++)
    {
      for (SizeType k{0}; k < gt.shape().at(2); k++)
      {
        for (SizeType l{0}; l < gt.shape().at(3); l++)
        {
          for (SizeType m{0}; m < gt.shape().at(4); m++)
          {
            gt.At(i, j, k, l, m) =
                input.At(begins.at(0) + i * strides.at(0), begins.at(1) + j * strides.at(1),
                         begins.at(2) + k * strides.at(2), begins.at(3) + l * strides.at(3),
                         begins.at(4) + m * strides.at(4));
          }
        }
      }
    }
  }

  OpType op(begins, ends, strides);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(input)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(input)});

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
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(input)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, strided_slice_saveparams_backward_batch_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::StridedSlice<TensorType>;
  using SPType     = typename OpType::SPType;
  using SizeType   = fetch::math::SizeType;
  using SizeVector = typename TypeParam::SizeVector;
  using DataType   = typename TypeParam::Type;

  TypeParam input({9, 9, 9, 6, 4});
  TypeParam error({6, 4, 3, 1, 2});

  SizeVector begins({3, 1, 0, 4, 0});
  SizeVector ends({8, 7, 8, 5, 2});
  SizeVector strides({1, 2, 3, 4, 2});

  auto     it  = error.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  fetch::ml::ops::StridedSlice<TypeParam> op(begins, ends, strides);
  std::vector<TypeParam>                  backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(input)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  backpropagated_signals = op.Backward({std::make_shared<TypeParam>(input)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TypeParam> new_backpropagated_signals =
      new_op.Backward({std::make_shared<TypeParam>(input)}, error);

  // test correct values
  EXPECT_TRUE(backpropagated_signals.at(0).AllClose(
      new_backpropagated_signals.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////////
/// SUBTRACT ///
////////////////

TYPED_TEST(SaveParamsTest, subtract_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Subtract<TensorType>::SPType;
  using OpType        = fetch::ml::ops::Subtract<TensorType>;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      " 8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  OpType op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

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
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, subtract_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpType     = fetch::ml::ops::Subtract<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "8,  7,-6, 5,-4, 3,-2, 1");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Subtract<TensorType> op;
  std::vector<TensorType>              prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

//////////////
/// SWITCH ///
//////////////

TYPED_TEST(SaveParamsTest, switch_saveparams_back_test_broadcast_mask)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = typename fetch::ml::ops::Switch<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType mask = TensorType::FromString("1, 1, 0");
  mask.Reshape({1, 3, 1});

  TensorType target_input = TensorType::FromString("3, 6, 2, 1, 3, -2, 2, 1, -9");
  target_input.Reshape({3, 3, 1});

  TensorType mask_value({3, 3, 1});
  mask_value.Fill(static_cast<DataType>(-100));

  TensorType error_signal = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9");
  error_signal.Reshape({3, 3, 1});

  fetch::ml::ops::Switch<TensorType> op;

  std::vector<TypeParam> prediction = op.Backward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(target_input),
       std::make_shared<const TensorType>(mask_value)},
      error_signal);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(target_input),
       std::make_shared<const TensorType>(mask_value)},
      error_signal);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TypeParam> new_prediction = new_op.Backward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(target_input),
       std::make_shared<const TensorType>(mask_value)},
      error_signal);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));

  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));

  EXPECT_TRUE(prediction.at(2).AllClose(
      new_prediction.at(2), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////
/// TANH ///
////////////

TYPED_TEST(SaveParamsTest, tanh_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::TanH<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::TanH<TensorType>;

  TensorType data = TypeParam::FromString("0, 0.2, 0.4, -0, -0.2, -0.4");

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
}

TYPED_TEST(SaveParamsTest, tanh_saveparams_backward_all_negative_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::TanH<TensorType>;
  using SPType     = typename OpType::SPType;

  uint8_t    n = 8;
  TypeParam  data{n};
  TypeParam  error{n};
  TensorType data_input  = TensorType::FromString("-0, -0.2, -0.4, -0.6, -0.8, -1.2, -1.4, -10");
  TensorType error_input = TensorType::FromString("-0.2, -0.1, -0.3, -0.2, -0.5, -0.1, -0.0, -0.3");

  for (uint64_t i(0); i < n; ++i)
  {
    data.Set(i, data_input[i]);
    error.Set(i, error_input[i]);
  }

  fetch::ml::ops::TanH<TypeParam> op;
  std::vector<TypeParam> prediction = op.Backward({std::make_shared<const TypeParam>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////
/// TOPK ///
////////////

TYPED_TEST(SaveParamsTest, top_k_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using SizeType      = fetch::math::SizeType;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::TopK<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::TopK<TensorType>;

  TensorType data = TypeParam::FromString("9,4,3,2;5,6,7,8;1,10,11,12;13,14,15,16");
  data.Reshape({4, 4});

  SizeType k      = 2;
  bool     sorted = true;

  OpType op(k, sorted);

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
}

TYPED_TEST(SaveParamsTest, top_k_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::TopK<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TypeParam::FromString("9,4,3,2;5,6,7,8;1,10,11,12;13,14,15,16");
  data.Reshape({4, 4});
  TensorType error = TypeParam::FromString("20,-21,22,-23;24,-25,26,-27");
  error.Reshape({2, 4});

  SizeType k      = 2;
  bool     sorted = true;

  fetch::ml::ops::TopK<TypeParam> op(k, sorted);

  // Run forward pass before backward pass
  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(data)}));
  op.Forward({std::make_shared<TypeParam>(data)}, prediction);

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

  // Run forward pass before backward pass
  new_op.Forward({std::make_shared<TypeParam>(data)}, prediction);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  fetch::math::state_clear<DataType>();
}

/////////////////
/// TRANSPOSE ///
/////////////////

TYPED_TEST(SaveParamsTest, transpose_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Transpose<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::Transpose<TensorType>;

  TensorType data = TypeParam::FromString(R"(1, 2, -3; 4, 5, 6)");

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
}

TYPED_TEST(SaveParamsTest, transpose_saveparams_backward_batch_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::Transpose<TensorType>;
  using SPType     = typename OpType::SPType;
  TypeParam a({4, 5, 2});
  TypeParam error({5, 4, 2});

  fetch::ml::ops::Transpose<TypeParam> op;
  std::vector<TypeParam>               backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  backpropagated_signals = op.Backward({std::make_shared<TypeParam>(a)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TypeParam> new_backpropagated_signals =
      new_op.Backward({std::make_shared<TypeParam>(a)}, error);

  // test correct values
  EXPECT_TRUE(backpropagated_signals.at(0).AllClose(
      new_backpropagated_signals.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(SaveParamsTest, weights_saveparams_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::Weights<TensorType>::SPType;
  using OpType     = typename fetch::ml::ops::Weights<TensorType>;

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  OpType op;
  op.SetData(data);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));

  op.Forward({}, prediction);

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
  new_op.Forward({}, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, weights_saveparams_gradient_step_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SizeType   = fetch::math::SizeType;
  using OpType     = typename fetch::ml::ops::Weights<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType       data(8);
  TensorType       error(8);
  std::vector<int> dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> errorInput({-1, 2, 3, -5, -8, 13, -21, -34});
  for (SizeType i{0}; i < 8; ++i)
  {
    data.Set(i, static_cast<DataType>(dataInput[i]));
    error.Set(i, static_cast<DataType>(errorInput[i]));
  }

  fetch::ml::ops::Weights<TensorType> op;
  op.SetData(data);

  TensorType prediction(op.ComputeOutputShape({}));
  op.Forward({}, prediction);

  std::vector<TensorType> error_signal = op.Backward({}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  op.Backward({}, error);

  TensorType grad = op.GetGradientsReferences();
  fetch::math::Multiply(grad, DataType{-1}, grad);
  op.ApplyGradient(grad);

  prediction = TensorType(op.ComputeOutputShape({}));
  op.Forward({}, prediction);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  new_op.Backward({}, error);

  TensorType new_grad = new_op.GetGradientsReferences();
  fetch::math::Multiply(new_grad, DataType{-1}, new_grad);
  new_op.ApplyGradient(new_grad);

  TensorType new_prediction = TensorType(new_op.ComputeOutputShape({}));
  new_op.Forward({}, new_prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(new_prediction,
                                  fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
