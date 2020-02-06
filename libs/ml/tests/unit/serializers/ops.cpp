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

#include "serializer_includes.hpp"
#include "serializer_test_utils.hpp"

namespace {

template <typename T>
class OpsSaveParamsTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(OpsSaveParamsTest, ::fetch::math::test::TensorFloatingTypes, );

///////////
/// OPS ///
///////////

///////////
/// ABS ///
///////////

TYPED_TEST(OpsSaveParamsTest, abs_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Abs<TensorType>::SPType;
  using OpKind        = fetch::ml::ops::Abs<TensorType>;

  TensorType data = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType gt = TensorType::FromString(
      "1, 2, 3, 4, 5, 6, 7, 8;"
      "1, 2, 3, 4, 5, 6, 7, 8");

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<TensorType const>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, abs_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpKind     = fetch::ml::ops::Abs<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType data = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Abs<TensorType> op;

  // run op once to make sure caches etc. have been filled. Otherwise the test might be trivial!
  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////
/// ADD ///
///////////

TYPED_TEST(OpsSaveParamsTest, add_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Add<TensorType>::SPType;
  using OpKind        = fetch::ml::ops::Add<TensorType>;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8;"
      "-8");

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, add_saveparams_backward_2D_broadcast_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using OpKind     = fetch::ml::ops::Add<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType({1, 1});
  data_2.At(0, 0)   = static_cast<DataType>(8);

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, 4;"
      "5, -5, 6, -6, 7, -7, 8, 8");

  fetch::ml::ops::Add<TensorType> op;
  std::vector<TensorType>         prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  // test correct values
  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////////////
/// AvgPool1D OP ///
////////////////////

TYPED_TEST(OpsSaveParamsTest, avg_pool_1d_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::AvgPool1D<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::AvgPool1D<TensorType>;
  using SizeType      = ::fetch::math::SizeType;

  TensorType data({2, 5, 2});
  TensorType data_input = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8, 9, -10");

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 5; ++j)
      {
        data(i, j, i_b) =
            data_input[i * 5 + j] + ::fetch::math::AsType<DataType>(static_cast<double>(i_b * 10));
      }
    }
  }

  OpKind op(4, 1);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, avg_pool_1d_saveparams_backward_test_2_channels)
{
  using TensorType = TypeParam;
  using SizeType   = ::fetch::math::SizeType;
  using OpKind     = typename fetch::ml::ops::AvgPool1D<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType data({2, 5, 2});
  TensorType error({2, 2, 2});
  TensorType data_input  = TensorType::FromString("1, -2, 3, -4, 10, -6, 7, -8, 9, -10");
  TensorType error_input = TensorType::FromString("2, 3, 4, 5");

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 5; ++j)
    {
      data(i, j, 0) = data_input[i * 5 + j];
    }
  }

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      error(i, j, 0) = error_input[i * 2 + j];
    }
  }

  fetch::ml::ops::AvgPool1D<TensorType> op(4, 1);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////////////
/// AvgPool2D OP ///
////////////////////

TYPED_TEST(OpsSaveParamsTest, avg_pool_2d_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::AvgPool2D<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::AvgPool2D<TensorType>;
  using SizeType      = ::fetch::math::SizeType;

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
        data(c, i, j, 0) = ::fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  OpKind op(3, 2);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, avg_pool_2d_saveparams_backward_2_channels_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = ::fetch::math::SizeType;
  using OpKind     = typename fetch::ml::ops::AvgPool2D<TensorType>;
  using SPType     = typename OpKind::SPType;

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
        data(c, i, j, 0) = ::fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < output_width; ++i)
    {
      for (SizeType j{0}; j < output_height; ++j)
      {
        error(c, i, j, 0) = ::fetch::math::AsType<DataType>((c + 1) * (1 + i + j));
      }
    }
  }

  fetch::ml::ops::AvgPool2D<TensorType> op(3, 2);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

//////////////////////
/// CONCATENATE OP ///
//////////////////////

TYPED_TEST(OpsSaveParamsTest, concatenate_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Concatenate<TensorType>::SPType;
  using OpKind        = fetch::ml::ops::Concatenate<TensorType>;

  TypeParam data1 = TypeParam::UniformRandom(64);
  TypeParam data2 = TypeParam::UniformRandom(64);
  data1.Reshape({8, 8});
  data2.Reshape({8, 8});

  OpKind op(1);

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data1), std::make_shared<const TensorType>(data2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data1), std::make_shared<const TensorType>(data2)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data1), std::make_shared<const TensorType>(data2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, concatenate_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpKind     = fetch::ml::ops::Concatenate<TensorType>;
  using SPType     = typename OpKind::SPType;

  TypeParam data1(std::vector<::fetch::math::SizeType>({8, 8}));
  TypeParam data2(std::vector<::fetch::math::SizeType>({8, 8}));

  fetch::ml::ops::Concatenate<TypeParam> op{1};

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}));
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, prediction);

  TypeParam              error_signal(prediction.shape());
  std::vector<TypeParam> gradients = op.Backward(
      {std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, error_signal);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TypeParam> new_gradients = new_op.Backward(
      {std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, error_signal);

  // test correct values
  EXPECT_TRUE(gradients.at(0).AllClose(
      new_gradients.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  EXPECT_TRUE(gradients.at(1).AllClose(
      new_gradients.at(1), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

/////////////////
/// CONV1D OP ///
/////////////////

TYPED_TEST(OpsSaveParamsTest, constant_saveable_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::Constant<TensorType>::SPType;
  using OpKind     = typename fetch::ml::ops::Constant<TensorType>;

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  OpKind op;
  op.SetData(data);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));

  op.Forward({}, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward({}, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

/////////////////
/// CONV1D OP ///
/////////////////

TYPED_TEST(OpsSaveParamsTest, conv1c_op_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Convolution1D<TensorType>::SPType;
  using OpKind        = fetch::ml::ops::Convolution1D<TensorType>;

  TensorType input({1, 1, 2});
  TensorType weights({1, 1, 1, 1});
  input.At(0, 0, 0)      = DataType{5};
  input.At(0, 0, 1)      = DataType{6};
  weights.At(0, 0, 0, 0) = DataType{-4};

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(input), std::make_shared<const TensorType>(weights)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(input), std::make_shared<const TensorType>(weights)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(input), std::make_shared<const TensorType>(weights)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, conv1d_op_saveparams_backward_3x3x2_5x3x3x2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = ::fetch::math::SizeType;
  using OpKind     = fetch::ml::ops::Convolution1D<TensorType>;
  using SPType     = typename OpKind::SPType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const batch_size      = 2;

  TensorType input({input_channels, input_height, batch_size});
  TensorType kernels({output_channels, input_channels, kernel_height, 1});
  TensorType error({output_channels, output_height, batch_size});

  // Generate input
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)
  {
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
    {
      for (SizeType i_i{0}; i_i < input_height; ++i_i)
      {
        input(i_ic, i_i, i_b) = ::fetch::math::AsType<DataType>(i_i + 1);
      }
    }
  }

  // Generate kernels
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
    {
      for (SizeType i_k{0}; i_k < kernel_height; ++i_k)
      {

        kernels(i_oc, i_ic, i_k, 0) = DataType{2};
      }
    }
  }

  // Generate error signal
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)
  {
    for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
    {
      for (SizeType i_o{0}; i_o < output_height; ++i_o)
      {
        error(i_oc, i_o, i_b) = static_cast<DataType>(i_o + 1);
      }
    }
  }

  fetch::ml::ops::Convolution1D<TensorType> op;
  std::vector<TensorType>                   prediction = op.Backward(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(kernels)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(kernels)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

/////////////////
/// CONV2D OP ///
/////////////////

TYPED_TEST(OpsSaveParamsTest, conv2d_op_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Convolution2D<TensorType>::SPType;
  using OpKind        = fetch::ml::ops::Convolution2D<TensorType>;
  using SizeType      = ::fetch::math::SizeType;

  TensorType input({3, 3, 3, 1});
  TensorType weights({1, 3, 3, 3, 1});
  SizeType   counter = 0;
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      for (SizeType k{0}; k < 3; ++k)
      {
        input.At(i, j, k, 0)      = static_cast<DataType>(counter);
        weights.At(0, i, j, k, 0) = static_cast<DataType>(counter);
        ++counter;
      }
    }
  }

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(input), std::make_shared<const TensorType>(weights)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(input), std::make_shared<const TensorType>(weights)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(input), std::make_shared<const TensorType>(weights)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, conv2d_op_saveparams_backward_3x3x3x2_5x3x3x3x2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = ::fetch::math::SizeType;
  using OpKind     = fetch::ml::ops::Convolution2D<TensorType>;
  using SPType     = typename OpKind::SPType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;

  SizeType const input_width  = 3;
  SizeType const input_height = 3;

  SizeType const kernel_width  = 3;
  SizeType const kernel_height = 3;

  SizeType const output_width  = 1;
  SizeType const output_height = 1;

  SizeType const batch_size = 2;

  TensorType input({input_channels, input_height, input_width, batch_size});
  TensorType kernels({output_channels, input_channels, kernel_height, kernel_width, 1});
  TensorType error({output_channels, output_height, output_width, batch_size});

  // Generate input
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)
  {
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
    {
      for (SizeType i_i{0}; i_i < input_height; ++i_i)
      {
        for (SizeType j_i{0}; j_i < input_width; ++j_i)
        {
          input(i_ic, i_i, j_i, i_b) = static_cast<DataType>(i_i + 1);
        }
      }
    }
  }

  // Generate kernels
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
    {
      for (SizeType i_k{0}; i_k < kernel_height; ++i_k)
      {
        for (SizeType j_k{0}; j_k < kernel_width; ++j_k)
        {
          kernels(i_oc, i_ic, i_k, j_k, 0) = DataType{2};
        }
      }
    }
  }

  // Generate error signal
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)
  {
    for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
    {
      for (SizeType i_o{0}; i_o < output_height; ++i_o)
      {
        for (SizeType j_o{0}; j_o < output_width; ++j_o)
        {
          error(i_oc, i_o, j_o, i_b) = static_cast<DataType>(i_o + 1);
        }
      }
    }
  }

  fetch::ml::ops::Convolution2D<TensorType> op;
  std::vector<TensorType>                   prediction = op.Backward(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(kernels)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(kernels)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  // test correct values
  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

//////////////
/// DIVIDE ///
//////////////

TYPED_TEST(OpsSaveParamsTest, divide_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Divide<TensorType>::SPType;
  using OpKind        = fetch::ml::ops::Divide<TensorType>;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      " 8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, divide_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpKind     = fetch::ml::ops::Divide<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "8,  7,-6, 5,-4, 3,-2, 1");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Divide<TensorType> op;
  std::vector<TensorType>            prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

//////////////////
/// EMBEDDINGS ///
//////////////////

TYPED_TEST(OpsSaveParamsTest, embeddings_saveparams_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::Embeddings<TensorType>::SPType;
  using OpKind     = fetch::ml::ops::Embeddings<TensorType>;

  TypeParam weights(std::vector<uint64_t>({6, 10}));

  for (uint32_t i(0); i < 10; ++i)
  {
    for (uint32_t j(0); j < 6; ++j)
    {
      weights(j, i) = typename TypeParam::Type(i * 10 + j);
    }
  }
  TypeParam input(std::vector<uint64_t>({2, 1}));
  input.At(0, 0) = typename TypeParam::Type(3);
  input.At(1, 0) = typename TypeParam::Type(5);

  OpKind op(6, 10);

  op.SetData(weights);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<TensorType const>(input)}));

  op.Forward({std::make_shared<TensorType const>(input)}, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<TensorType const>(input)}));
  new_op.Forward({std::make_shared<TensorType const>(input)}, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, embeddings_saveparams_backward)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpKind     = fetch::ml::ops::Embeddings<TensorType>;
  using SPType     = typename OpKind::SPType;

  fetch::ml::ops::Embeddings<TypeParam> op(6, 10);
  TypeParam                             weights(std::vector<uint64_t>({6, 10}));
  for (uint32_t i{0}; i < 10; ++i)
  {
    for (uint32_t j{0}; j < 6; ++j)
    {
      weights(j, i) = static_cast<DataType>(i * 10 + j);
    }
  }

  op.SetData(weights);

  TensorType input(std::vector<uint64_t>({2, 1}));
  input.At(0, 0) = DataType{3};
  input.At(1, 0) = DataType{5};

  TensorType output(op.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  op.Forward({std::make_shared<TypeParam>(input)}, output);

  TensorType error_signal(std::vector<uint64_t>({6, 2, 1}));
  for (uint32_t j{0}; j < 2; ++j)
  {
    for (uint32_t k{0}; k < 6; ++k)
    {
      error_signal(k, j, 0) = static_cast<DataType>(j * 6 + k);
    }
  }

  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<TypeParam>(input)}, error_signal);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<TypeParam>(input)}, error_signal);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////
/// EXP ///
///////////

TYPED_TEST(OpsSaveParamsTest, exp_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Exp<TensorType>::SPType;
  using OpKind        = fetch::ml::ops::Exp<TensorType>;

  TensorType data = TensorType::FromString(
      " 0, -2, 3,-4, 5,-6, 7,-8;"
      "-1,  2,-3, 4,-5, 6,-7, 8");

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, exp_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpKind     = fetch::ml::ops::Exp<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType data = TensorType::FromString(
      "0, -2, 3,-4, 5,-6, 7,-8;"
      "-1,  2,-3, 4,-5, 6,-7, 8");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Exp<TensorType> op;
  std::vector<TensorType>         prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(OpsSaveParamsTest, flatten_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Flatten<TensorType>::SPType;
  using OpKind        = fetch::ml::ops::Flatten<TensorType>;

  using SizeType = ::fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType height  = 7;
  SizeType width   = 6;
  SizeType batches = 5;

  TypeParam data(std::vector<uint64_t>({height, width, batches}));

  for (SizeType i{0}; i < height; i++)
  {
    for (SizeType j{0}; j < width; j++)
    {
      for (SizeType n{0}; n < batches; n++)
      {
        data(i, j, n) = static_cast<DataType>(i * 100 + j * 10 + n);
      }
    }
  }

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, flatten_saveparams_backward_test)
{
  using SizeType   = ::fetch::math::SizeType;
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using OpKind     = fetch::ml::ops::Flatten<TensorType>;
  using SPType     = typename OpKind::SPType;

  SizeType height  = 5;
  SizeType width   = 6;
  SizeType batches = 7;

  TypeParam data({height, width, batches});
  TypeParam error_signal({height * width, batches});

  for (SizeType i{0}; i < height; i++)
  {
    for (SizeType j{0}; j < width; j++)
    {
      for (SizeType n{0}; n < batches; n++)
      {
        data(i, j, n)                   = static_cast<DataType>(-1);
        error_signal(j * height + i, n) = static_cast<DataType>(i * 100 + j * 10 + n);
      }
    }
  }

  fetch::ml::ops::Flatten<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TypeParam>(data)}));
  op.Forward({std::make_shared<const TypeParam>(data)}, prediction);

  std::vector<TypeParam> gradients =
      op.Backward({std::make_shared<const TypeParam>(data)}, error_signal);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TypeParam> new_gradients =
      new_op.Backward({std::make_shared<const TypeParam>(data)}, error_signal);

  // test correct values
  EXPECT_TRUE(gradients.at(0).AllClose(
      new_gradients.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

/////////////////////
/// LAYER NORM OP ///
/////////////////////

TYPED_TEST(OpsSaveParamsTest, layer_norm_op_saveparams_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::LayerNorm<TensorType>::SPType;
  using OpKind     = fetch::ml::ops::LayerNorm<TensorType>;

  TensorType data = TensorType::FromString(
      "1, 2, 3, 0;"
      "2, 3, 2, 1;"
      "3, 6, 4, 13");

  data.Reshape({3, 2, 2});
  auto s1 = data.View(0).ToString();
  auto s2 = data.View(1).ToString();

  fetch::ml::ops::LayerNorm<TensorType> op(static_cast<typename TypeParam::SizeType>(0));

  TensorType prediction(op.ComputeOutputShape({std::make_shared<TensorType>(data)}));
  op.Forward({std::make_shared<TensorType>(data)}, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<TensorType>(data)}));
  op.Forward({std::make_shared<TensorType>(data)}, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, layer_norm_op_saveparams_backward_test_3d)
{
  using TensorType = TypeParam;
  using OpKind     = fetch::ml::ops::LayerNorm<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType data = TensorType::FromString(
      "1, 1, 0.5, 2;"
      "2, 0, 3, 1;"
      "1, 1, 7, 9");
  data.Reshape({3, 2, 2});

  TensorType error_signal = TensorType::FromString(
      "-1, 2, 1, 1;"
      "2, 0, 1, 3;"
      "1, 1, 1, 6");
  error_signal.Reshape({3, 2, 2});

  fetch::ml::ops::LayerNorm<TensorType> op(static_cast<typename TypeParam::SizeType>(0));

  auto prediction = op.Backward({std::make_shared<TensorType>(data)}, error_signal);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  auto new_prediction = new_op.Backward({std::make_shared<TensorType>(data)}, error_signal);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////
/// LOG ///
///////////

TYPED_TEST(OpsSaveParamsTest, log_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Log<TensorType>::SPType;
  using OpKind        = fetch::ml::ops::Log<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
  ::fetch::math::state_clear<DataType>();
}

TYPED_TEST(OpsSaveParamsTest, log_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpKind     = fetch::ml::ops::Log<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType data  = TensorType::FromString("1, -2, 4, -10, 100");
  TensorType error = TensorType::FromString("1, 1, 1, 2, 0");

  fetch::ml::ops::Log<TypeParam> op;

  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  ::fetch::math::state_clear<DataType>();
}

/////////////////
/// MASK FILL ///
/////////////////

TYPED_TEST(OpsSaveParamsTest, mask_fill_saveparams_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using OpKind     = typename fetch::ml::ops::MaskFill<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType mask = TensorType::FromString("1, 0, 1, 0, 0, 0, 0, 1, 1");
  mask.Reshape({3, 3, 1});

  TensorType then_array = TensorType::FromString("3, 6, 2, 1, 3, -2, 2, 1, -9");
  then_array.Reshape({3, 3, 1});

  fetch::ml::ops::MaskFill<TensorType> op(static_cast<DataType>(-100));

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(then_array)}));
  op.Forward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(then_array)},
      prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old

  TypeParam new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(then_array)}));
  new_op.Forward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(then_array)},
      new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, mask_fill_saveparams_back_test_broadcast_mask)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpKind     = typename fetch::ml::ops::MaskFill<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType mask = TensorType::FromString("1, 1, 0");
  mask.Reshape({1, 3, 1});

  TensorType target_input = TensorType::FromString("3, 6, 2, 1, 3, -2, 2, 1, -9");
  target_input.Reshape({3, 3, 1});

  TensorType error_signal = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9");
  error_signal.Reshape({3, 3, 1});

  fetch::ml::ops::MaskFill<TensorType> op(static_cast<DataType>(-100));

  std::vector<TypeParam> prediction = op.Backward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(target_input)},
      error_signal);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(target_input)},
      error_signal);

  // test correct values
  EXPECT_TRUE(prediction.at(0) == new_prediction.at(0));

  EXPECT_TRUE(prediction.at(1) == new_prediction.at(1));
}

///////////////////////
/// MATRIX MULTIPLY ///
///////////////////////

TYPED_TEST(OpsSaveParamsTest, matrix_multiply_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MatrixMultiply<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::MatrixMultiply<TensorType>;

  TypeParam data_1 = TypeParam::FromString("1, 2, -3, 4, 5");
  TypeParam data_2 = TypeParam::FromString(
      "-11, 12, 13, 14; 21, 22, 23, 24; 31, 32, 33, 34; 41, 42, 43, 44; 51, 52, 53, 54");

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, matrix_multiply_saveparams_backward_batch_test)
{
  using TensorType = TypeParam;
  using OpKind     = typename fetch::ml::ops::MatrixMultiply<TensorType>;
  using SPType     = typename OpKind::SPType;
  TypeParam a1({3, 4, 2});
  TypeParam b1({4, 3, 2});
  TypeParam error({3, 3, 2});
  TypeParam gradient_a({3, 4, 2});
  TypeParam gradient_b({4, 3, 2});

  fetch::ml::ops::MatrixMultiply<TypeParam> op;
  std::vector<TypeParam>                    backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a1), std::make_shared<TypeParam>(b1)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TypeParam> new_backpropagated_signals =
      new_op.Backward({std::make_shared<TypeParam>(a1), std::make_shared<TypeParam>(b1)}, error);

  // test correct values
  EXPECT_TRUE(backpropagated_signals.at(0).AllClose(
      new_backpropagated_signals.at(0),
      ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));

  EXPECT_TRUE(backpropagated_signals.at(1).AllClose(
      new_backpropagated_signals.at(1),
      ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////////////
/// MAX POOL ///
///////////////////

TYPED_TEST(OpsSaveParamsTest, maxpool_saveparams_test_1d)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MaxPool1D<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::MaxPool1D<TensorType>;
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
        data(i, j, i_b) = ::fetch::math::AsType<DataType>(data_input[i * 5 + j]) +
                          ::fetch::math::AsType<DataType>(i_b * 10);
      }
    }

    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 2; ++j)
      {
        gt(i, j, i_b) = ::fetch::math::AsType<DataType>(gt_input[i * 2 + j]) +
                        ::fetch::math::AsType<DataType>(i_b * 10);
      }
    }
  }

  OpKind op(4, 1);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, maxpool_saveparams_backward_test_1d_2_channels)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;
  using OpKind     = typename fetch::ml::ops::MaxPool1D<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType          data({2, 5, 2});
  TensorType          error({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 10, -6, 7, -8, 9, -10});
  std::vector<double> errorInput({2, 3, 4, 5});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 5; ++j)
    {
      data(i, j, 0) = ::fetch::math::AsType<DataType>(data_input[i * 5 + j]);
    }
  }

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      error(i, j, 0) = ::fetch::math::AsType<DataType>(errorInput[i * 2 + j]);
    }
  }

  fetch::ml::ops::MaxPool1D<TensorType> op(4, 1);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(OpsSaveParamsTest, maxpool_saveparams_test_2d)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MaxPool2D<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::MaxPool2D<TensorType>;
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
        data(c, i, j, 0) = ::fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  OpKind op(3, 2);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, maxpool_saveparams_backward_2_channels_test_2d)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;
  using OpKind     = typename fetch::ml::ops::MaxPool2D<TensorType>;
  using SPType     = typename OpKind::SPType;

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
        data(c, i, j, 0) = ::fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < output_width; ++i)
    {
      for (SizeType j{0}; j < output_height; ++j)
      {
        error(c, i, j, 0) = ::fetch::math::AsType<DataType>((c + 1) * (1 + i + j));
      }
    }
  }

  fetch::ml::ops::MaxPool2D<TensorType> op(3, 2);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////////////
/// MAX POOL 1D ///
///////////////////

TYPED_TEST(OpsSaveParamsTest, maxpool_1d_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MaxPool1D<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::MaxPool1D<TensorType>;
  using SizeType      = ::fetch::math::SizeType;

  TensorType          data({2, 5, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 5; ++j)
      {
        data(i, j, i_b) = ::fetch::math::AsType<DataType>(data_input[i * 5 + j]) +
                          ::fetch::math::AsType<DataType>(i_b * 10);
      }
    }
  }

  OpKind op(4, 1);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, maxpool_1d_saveparams_backward_test_2_channels)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = ::fetch::math::SizeType;
  using OpKind     = typename fetch::ml::ops::MaxPool1D<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType          data({2, 5, 2});
  TensorType          error({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 10, -6, 7, -8, 9, -10});
  std::vector<double> errorInput({2, 3, 4, 5});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 5; ++j)
    {
      data(i, j, 0) = ::fetch::math::AsType<DataType>(data_input[i * 5 + j]);
    }
  }

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      error(i, j, 0) = ::fetch::math::AsType<DataType>(errorInput[i * 2 + j]);
    }
  }

  fetch::ml::ops::MaxPool1D<TensorType> op(4, 1);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////////////
/// MAX POOL 2D ///
///////////////////

TYPED_TEST(OpsSaveParamsTest, maxpool_2d_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MaxPool2D<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::MaxPool2D<TensorType>;
  using SizeType      = ::fetch::math::SizeType;

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
        data(c, i, j, 0) = ::fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  OpKind op(3, 2);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, maxpool_2d_saveparams_backward_2_channels_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = ::fetch::math::SizeType;
  using OpKind     = typename fetch::ml::ops::MaxPool2D<TensorType>;
  using SPType     = typename OpKind::SPType;

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
        data(c, i, j, 0) = ::fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < output_width; ++i)
    {
      for (SizeType j{0}; j < output_height; ++j)
      {
        error(c, i, j, 0) = ::fetch::math::AsType<DataType>((c + 1) * (1 + i + j));
      }
    }
  }

  fetch::ml::ops::MaxPool2D<TensorType> op(3, 2);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////////
/// MAXIMUM ///
///////////////

TYPED_TEST(OpsSaveParamsTest, maximum_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Maximum<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::Maximum<TensorType>;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, maximum_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpKind     = typename fetch::ml::ops::Maximum<TensorType>;
  using SPType     = typename OpKind::SPType;

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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////////
/// MULTIPLY ///
////////////////

TYPED_TEST(OpsSaveParamsTest, multiply_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Multiply<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::Multiply<TensorType>;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, multiply_saveparams_backward_test_NB_NB)
{
  using TensorType = TypeParam;
  using OpKind     = typename fetch::ml::ops::Multiply<TensorType>;
  using SPType     = typename OpKind::SPType;

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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));

  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));

  ASSERT_TRUE(!::fetch::math::state_overflow<typename TypeParam::Type>());
}

///////////////
/// ONE-HOT ///
///////////////

TYPED_TEST(OpsSaveParamsTest, one_hot_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using SizeType      = typename TypeParam::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::OneHot<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::OneHot<TensorType>;

  TensorType data = TypeParam::FromString("1,0,1,2");
  data.Reshape({2, 2, 1, 1});

  SizeType depth     = 3;
  SizeType axis      = 3;
  auto     on_value  = DataType{5};
  auto     off_value = DataType{-1};

  OpKind op(depth, axis, on_value, off_value);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

///////////////////
/// PLACEHOLDER ///
///////////////////

TYPED_TEST(OpsSaveParamsTest, placeholder_saveable_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::PlaceHolder<TensorType>::SPType;
  using OpKind     = typename fetch::ml::ops::PlaceHolder<TensorType>;

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  OpKind op;
  op.SetData(data);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));

  op.Forward({}, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

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

TYPED_TEST(OpsSaveParamsTest, prelu_op_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::PReluOp<TensorType>::SPType;
  using OpKind        = fetch::ml::ops::PReluOp<TensorType>;

  TensorType data =
      TensorType::FromString("1, -2, 3,-4, 5,-6, 7,-8; -1,  2,-3, 4,-5, 6,-7, 8").Transpose();

  TensorType alpha = TensorType::FromString("0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8").Transpose();

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}));
  VecTensorType vec_data({std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, prelu_op_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpKind     = fetch::ml::ops::PReluOp<TensorType>;
  using SPType     = typename OpKind::SPType;

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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

/////////////////////////
/// REDUCE MEAN TESTS ///
/////////////////////////

TYPED_TEST(OpsSaveParamsTest, reduce_mean_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::ReduceMean<TensorType>::SPType;
  using OpKind        = fetch::ml::ops::ReduceMean<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});

  OpKind op(1);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
  ::fetch::math::state_clear<DataType>();
}

TYPED_TEST(OpsSaveParamsTest, reduce_mean_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpKind     = fetch::ml::ops::ReduceMean<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});
  TensorType error = TensorType::FromString("1, -2, -1, 2");
  error.Reshape({2, 1, 2});

  fetch::ml::ops::ReduceMean<TypeParam> op(1);

  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  ::fetch::math::state_clear<DataType>();
}

TYPED_TEST(OpsSaveParamsTest, ReduceMean_graph_serialization_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = ::fetch::ml::GraphSaveableParams<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});

  ::fetch::ml::Graph<TensorType> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string output_name =
      g.template AddNode<fetch::ml::ops::ReduceMean<TensorType>>("Output", {input_name}, 1);

  g.SetInput(input_name, data);
  TypeParam output = g.Evaluate("Output");

  // extract saveparams
  SPType gsp = g.GetGraphSaveableParams();

  ::fetch::serializers::MsgPackSerializer b;
  b << gsp;

  // deserialize
  b.seek(0);
  SPType dsp2;
  b >> dsp2;

  // rebuild graph
  auto new_graph_ptr = std::make_shared<::fetch::ml::Graph<TensorType>>();
  ::fetch::ml::utilities::BuildGraph(gsp, new_graph_ptr);

  new_graph_ptr->SetInput(input_name, data);
  TypeParam output2 = new_graph_ptr->Evaluate("Output");

  // Test correct values
  ASSERT_EQ(output.shape(), output2.shape());
  ASSERT_TRUE(output.AllClose(output2, ::fetch::math::function_tolerance<DataType>(),
                              ::fetch::math::function_tolerance<DataType>()));
}

/////////////////////
/// RESHAPE TESTS ///
/////////////////////

TYPED_TEST(OpsSaveParamsTest, Reshape_graph_serialisation_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = ::fetch::ml::GraphSaveableParams<TensorType>;

  std::vector<::fetch::math::SizeType> final_shape({8, 1, 1, 1});

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2, 1});

  ::fetch::ml::Graph<TensorType> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string output_name =
      g.template AddNode<fetch::ml::ops::Reshape<TensorType>>("Output", {input_name}, final_shape);

  g.SetInput(input_name, data);
  TypeParam output = g.Evaluate("Output");

  // extract saveparams
  SPType gsp = g.GetGraphSaveableParams();

  ::fetch::serializers::MsgPackSerializer b;
  b << gsp;

  // deserialize
  b.seek(0);
  SPType dsp2;
  b >> dsp2;

  // rebuild graph
  auto new_graph_ptr = std::make_shared<::fetch::ml::Graph<TensorType>>();
  ::fetch::ml::utilities::BuildGraph(gsp, new_graph_ptr);

  new_graph_ptr->SetInput(input_name, data);
  TypeParam output2 = new_graph_ptr->Evaluate("Output");

  // Test correct values
  ASSERT_EQ(output.shape(), output2.shape());
  ASSERT_TRUE(output.AllClose(output2, ::fetch::math::function_tolerance<DataType>(),
                              ::fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(OpsSaveParamsTest, reshape_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Reshape<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::Reshape<TensorType>;

  // construct tensor & reshape op
  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2, 1});
  fetch::ml::ops::Reshape<TensorType> op({8, 1, 1, 1});

  // forward pass
  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});
  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, reshape_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpKind     = fetch::ml::ops::Reshape<TensorType>;
  using SPType     = typename OpKind::SPType;

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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  ::fetch::math::state_clear<DataType>();
}

/////////////
/// SLICE ///
/////////////

TYPED_TEST(OpsSaveParamsTest, slice_single_axis_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using OpKind        = typename fetch::ml::ops::Slice<TensorType>;
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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, slice_single_axis_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpKind     = fetch::ml::ops::Slice<TensorType>;
  using SPType     = typename OpKind::SPType;
  using SizeType   = ::fetch::math::SizeType;

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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  ::fetch::math::state_clear<DataType>();
}

TYPED_TEST(OpsSaveParamsTest, slice_ranged_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using OpKind        = typename fetch::ml::ops::Slice<TensorType>;
  using SPType        = typename OpKind::SPType;
  using SizeType      = ::fetch::math::SizeType;
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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, slice_ranged_saveparams_backward_test)
{
  using TensorType   = TypeParam;
  using DataType     = typename TypeParam::Type;
  using OpKind       = fetch::ml::ops::Slice<TensorType>;
  using SPType       = typename OpKind::SPType;
  using SizeType     = ::fetch::math::SizeType;
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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  ::fetch::math::state_clear<DataType>();
}

///////////////////
/// SLICE TESTS ///
///////////////////

TYPED_TEST(OpsSaveParamsTest, slice_multi_axes_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using OpKind        = typename fetch::ml::ops::Slice<TensorType>;
  using SPType        = typename OpKind::SPType;
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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

////////////
/// SQRT ///
////////////

TYPED_TEST(OpsSaveParamsTest, sqrt_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Sqrt<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::Sqrt<TensorType>;

  TensorType data = TensorType::FromString("0, 1, 2, 4, 10, 100");

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, sqrt_saveparams_backward_all_positive_test)
{
  using TensorType = TypeParam;
  using OpKind     = typename fetch::ml::ops::Sqrt<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType data  = TensorType::FromString("1,   2,         4,   10,       100");
  TensorType error = TensorType::FromString("1,   1,         1,    2,         0");

  fetch::ml::ops::Sqrt<TypeParam> op;

  // run op once to make sure caches etc. have been filled. Otherwise the test might be trivial!
  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

/////////////////////
/// SQUEEZE TESTS ///
/////////////////////

TYPED_TEST(OpsSaveParamsTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Squeeze<TensorType>::SPType;
  using OpKind        = fetch::ml::ops::Squeeze<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");
  data.Reshape({6, 1});

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
  ::fetch::math::state_clear<DataType>();
}

TYPED_TEST(OpsSaveParamsTest, saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpKind     = fetch::ml::ops::Squeeze<TensorType>;
  using SPType     = typename OpKind::SPType;

  TensorType data = TensorType::FromString("1, -2, 4, -10, 100");
  data.Reshape({1, 5});
  TensorType error = TensorType::FromString("1, 1, 1, 2, 0");
  error.Reshape({5});

  fetch::ml::ops::Squeeze<TypeParam> op;

  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  ::fetch::math::state_clear<DataType>();
}

TYPED_TEST(OpsSaveParamsTest, squeeze_graph_serialization_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = ::fetch::ml::GraphSaveableParams<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");
  data.Reshape({6, 1});

  ::fetch::ml::Graph<TensorType> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string output_name =
      g.template AddNode<fetch::ml::ops::Squeeze<TensorType>>("Output", {input_name});

  g.SetInput(input_name, data);
  TypeParam output = g.Evaluate("Output");

  // extract saveparams
  SPType gsp = g.GetGraphSaveableParams();

  ::fetch::serializers::MsgPackSerializer b;
  b << gsp;

  // deserialize
  b.seek(0);
  SPType dsp2;
  b >> dsp2;

  // rebuild graph
  auto new_graph_ptr = std::make_shared<::fetch::ml::Graph<TensorType>>();
  ::fetch::ml::utilities::BuildGraph(gsp, new_graph_ptr);

  new_graph_ptr->SetInput(input_name, data);
  TypeParam output2 = new_graph_ptr->Evaluate("Output");

  // Test correct values
  ASSERT_EQ(output.shape(), output2.shape());
  ASSERT_TRUE(output.AllClose(output2, ::fetch::math::function_tolerance<DataType>(),
                              ::fetch::math::function_tolerance<DataType>()));
}

/////////////////////
/// STRIDED SLICE ///
/////////////////////

TYPED_TEST(OpsSaveParamsTest, strided_slice_saveparams_test)
{
  using TensorType    = TypeParam;
  using SizeType      = ::fetch::math::SizeType;
  using SizeVector    = typename TypeParam::SizeVector;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::StridedSlice<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::StridedSlice<TensorType>;

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

  OpKind op(begins, ends, strides);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(input)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(input)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(input)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, strided_slice_saveparams_backward_batch_test)
{
  using TensorType = TypeParam;
  using OpKind     = typename fetch::ml::ops::StridedSlice<TensorType>;
  using SPType     = typename OpKind::SPType;
  using SizeType   = ::fetch::math::SizeType;
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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TypeParam> new_backpropagated_signals =
      new_op.Backward({std::make_shared<TypeParam>(input)}, error);

  // test correct values
  EXPECT_TRUE(backpropagated_signals.at(0).AllClose(
      new_backpropagated_signals.at(0),
      ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////////
/// SUBTRACT ///
////////////////

TYPED_TEST(OpsSaveParamsTest, subtract_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Subtract<TensorType>::SPType;
  using OpKind        = fetch::ml::ops::Subtract<TensorType>;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      " 8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, subtract_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpKind     = fetch::ml::ops::Subtract<TensorType>;
  using SPType     = typename OpKind::SPType;

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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

//////////////
/// SWITCH ///
//////////////

TYPED_TEST(OpsSaveParamsTest, switch_saveparams_back_test_broadcast_mask)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpKind     = typename fetch::ml::ops::Switch<TensorType>;
  using SPType     = typename OpKind::SPType;

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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TypeParam> new_prediction = new_op.Backward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(target_input),
       std::make_shared<const TensorType>(mask_value)},
      error_signal);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));

  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));

  EXPECT_TRUE(prediction.at(2).AllClose(
      new_prediction.at(2), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////
/// TANH ///
////////////

TYPED_TEST(OpsSaveParamsTest, tanh_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::TanH<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::TanH<TensorType>;

  TensorType data = TypeParam::FromString("0, 0.2, 0.4, -0, -0.2, -0.4");

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, tanh_saveparams_backward_all_negative_test)
{
  using TensorType = TypeParam;
  using OpKind     = typename fetch::ml::ops::TanH<TensorType>;
  using SPType     = typename OpKind::SPType;

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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////
/// TOPK ///
////////////

TYPED_TEST(OpsSaveParamsTest, top_k_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using SizeType      = ::fetch::math::SizeType;
  using SizeType      = ::fetch::math::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::TopK<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::TopK<TensorType>;

  TensorType data = TypeParam::FromString("9,4,3,2;5,6,7,8;1,10,11,12;13,14,15,16");
  data.Reshape({4, 4});

  SizeType k      = 2;
  bool     sorted = true;

  OpKind op(k, sorted);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, top_k_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;
  using DataType   = typename TypeParam::Type;
  using OpKind     = fetch::ml::ops::TopK<TensorType>;
  using SPType     = typename OpKind::SPType;

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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // Run forward pass before backward pass
  new_op.Forward({std::make_shared<TypeParam>(data)}, prediction);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
  ::fetch::math::state_clear<DataType>();
}

/////////////////
/// TRANSPOSE ///
/////////////////

TYPED_TEST(OpsSaveParamsTest, transpose_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Transpose<TensorType>::SPType;
  using OpKind        = typename fetch::ml::ops::Transpose<TensorType>;

  TensorType data = TypeParam::FromString(R"(1, 2, -3; 4, 5, 6)");

  OpKind op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, transpose_saveparams_backward_batch_test)
{
  using TensorType = TypeParam;
  using OpKind     = typename fetch::ml::ops::Transpose<TensorType>;
  using SPType     = typename OpKind::SPType;
  TypeParam a({4, 5, 2});
  TypeParam error({5, 4, 2});

  fetch::ml::ops::Transpose<TypeParam> op;
  std::vector<TypeParam>               backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a)}, error);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);
  // check that new predictions match the old
  std::vector<TypeParam> new_backpropagated_signals =
      new_op.Backward({std::make_shared<TypeParam>(a)}, error);

  // test correct values
  EXPECT_TRUE(backpropagated_signals.at(0).AllClose(
      new_backpropagated_signals.at(0),
      ::fetch::math::function_tolerance<typename TypeParam::Type>(),
      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(OpsSaveParamsTest, weights_saveparams_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::Weights<TensorType>::SPType;
  using OpKind     = typename fetch::ml::ops::Weights<TensorType>;

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  OpKind op;
  op.SetData(data);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));

  op.Forward({}, prediction);

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward({}, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(OpsSaveParamsTest, weights_saveparams_gradient_step_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SizeType   = ::fetch::math::SizeType;
  using OpKind     = typename fetch::ml::ops::Weights<TensorType>;
  using SPType     = typename OpKind::SPType;

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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
  OpKind new_op(*dsp);

  // make another prediction with the original op
  // check that new predictions match the old
  op.Backward({}, error);
  new_op.Backward({}, error);

  TensorType grad     = op.GetGradientsReferences();
  TensorType new_grad = new_op.GetGradientsReferences();

  ::fetch::math::Multiply(grad, DataType{-1}, grad);
  ::fetch::math::Multiply(new_grad, DataType{-1}, new_grad);

  op.ApplyGradient(grad);
  new_op.ApplyGradient(new_grad);

  prediction                = TensorType(op.ComputeOutputShape({}));
  TensorType new_prediction = TensorType(new_op.ComputeOutputShape({}));

  op.Forward({}, prediction);
  new_op.Forward({}, new_prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(new_prediction,
                                  ::fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  ::fetch::math::function_tolerance<typename TypeParam::Type>()));
}

}  // namespace
