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
#include "ml/ops/max_pool.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class MaxPoolTest : public ::testing::Test
{
};

TYPED_TEST_CASE(MaxPoolTest, math::test::TensorFloatingTypes);

TYPED_TEST(MaxPoolTest, forward_test_1d_3_2_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;

  TensorType          data({1, 10, 2});
  TensorType          gt({1, 4, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gt_input({3, 5, 7, 9});

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 10; ++i)
    {
      data(0, i, i_b) =
          fetch::math::AsType<DataType>(data_input[i]) + fetch::math::AsType<DataType>(i_b * 10);
    }
    for (SizeType i{0}; i < 4; ++i)
    {
      gt(0, i, i_b) =
          fetch::math::AsType<DataType>(gt_input[i]) + fetch::math::AsType<DataType>(i_b * 10);
    }
  }

  fetch::ml::ops::MaxPool<TensorType> op(3, 2);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(MaxPoolTest, backward_test_1d)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;

  TensorType          data({1, 10, 2});
  TensorType          error({1, 4, 2});
  TensorType          gt({1, 10, 2});
  std::vector<double> data_input({1, -2, 3, -4, 10, -6, 7, -8, 9, -10});
  std::vector<double> errorInput({2, 3, 4, 5});
  std::vector<double> gt_input1({0, 0, 2, 0, 7, 0, 0, 0, 5, 0});
  std::vector<double> gt_input2({0, 0, 3, 0, 9, 0, 0, 0, 6, 0});

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 10; ++i)
    {
      data(0, i, i_b) =
          fetch::math::AsType<DataType>(data_input[i]) + fetch::math::AsType<DataType>(i_b);
    }
    for (SizeType i{0}; i < 4; ++i)
    {
      error(0, i, i_b) =
          fetch::math::AsType<DataType>(errorInput[i]) + fetch::math::AsType<DataType>(i_b);
    }
  }

  for (SizeType i{0}; i < 10; ++i)
  {
    gt(0, i, 0) = fetch::math::AsType<DataType>(gt_input1[i]);
    gt(0, i, 1) = fetch::math::AsType<DataType>(gt_input2[i]);
  }

  fetch::ml::ops::MaxPool<TensorType> op(3, 2);
  std::vector<TensorType>             prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(
      prediction.at(0).AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(MaxPoolTest, backward_test_1d_2_channels)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;

  TensorType          data({2, 5, 2});
  TensorType          error({2, 2, 2});
  TensorType          gt({2, 5, 2});
  std::vector<double> data_input({1, -2, 3, -4, 10, -6, 7, -8, 9, -10});
  std::vector<double> errorInput({2, 3, 4, 5});
  std::vector<double> gt_input({0, 0, 2, 0, 3, 0, 0, 0, 9, 0});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 5; ++j)
    {
      data(i, j, 0) = fetch::math::AsType<DataType>(data_input[i * 5 + j]);
      gt(i, j, 0)   = fetch::math::AsType<DataType>(gt_input[i * 5 + j]);
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

  // test correct values
  EXPECT_TRUE(
      prediction.at(0).AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(MaxPoolTest, forward_test_1d_4_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;

  TensorType          data({1, 10, 1});
  TensorType          gt({1, 4, 1});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gt_input({3, 5, 7, 9});
  for (SizeType i{0}; i < 10; ++i)
  {
    data(0, i, 0) = fetch::math::AsType<DataType>(data_input[i]);
  }

  for (SizeType i{0}; i < 4; ++i)
  {
    gt(0, i, 0) = fetch::math::AsType<DataType>(gt_input[i]);
  }

  fetch::ml::ops::MaxPool<TensorType> op(4, 2);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(MaxPoolTest, forward_test_1d_2_channels_4_1_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;

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

  fetch::ml::ops::MaxPool<TensorType> op(4, 1);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(MaxPoolTest, forward_test_1d_2_4_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;

  TensorType          data({1, 10, 2});
  TensorType          gt({1, 3, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gt_input({1, 5, 9});
  for (SizeType i{0}; i < 10; ++i)
  {
    data(0, i, 0) = fetch::math::AsType<DataType>(data_input[i]);
  }

  for (SizeType i{0}; i < 3; ++i)
  {
    gt(0, i, 0) = fetch::math::AsType<DataType>(gt_input[i]);
  }

  fetch::ml::ops::MaxPool<TensorType> op(2, 4);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(MaxPoolTest, saveparams_test_1d)
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

TYPED_TEST(MaxPoolTest, saveparams_backward_test_1d_2_channels)
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

TYPED_TEST(MaxPoolTest, forward_test_2d_3_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;

  SizeType const input_width  = 10;
  SizeType const input_height = 5;

  SizeType const output_width  = 4;
  SizeType const output_height = 2;

  SizeType const batch_size = 2;

  TensorType          data({1, input_width, input_height, batch_size});
  TensorType          gt({1, output_width, output_height, batch_size});
  std::vector<double> gt_input({4, 8, 12, 16, 8, 16, 24, 32});
  for (SizeType i{0}; i < input_width; ++i)
  {
    for (SizeType j{0}; j < input_height; ++j)
    {
      data(0, i, j, 0) = fetch::math::AsType<DataType>(i * j);
    }
  }

  for (SizeType i{0}; i < output_width; ++i)
  {
    for (SizeType j{0}; j < output_height; ++j)
    {
      gt(0, i, j, 0) = fetch::math::AsType<DataType>(gt_input[i + j * output_width]);
    }
  }

  fetch::ml::ops::MaxPool2D<TensorType> op(3, 2);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(MaxPoolTest, forward_2_channels_test_2d_3_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 10;
  SizeType const input_height  = 5;

  SizeType const output_width  = 4;
  SizeType const output_height = 2;

  SizeType const batch_size = 2;

  TensorType          data({channels_size, input_width, input_height, batch_size});
  TensorType          gt({channels_size, output_width, output_height, batch_size});
  std::vector<double> gt_input({4, 8, 12, 16, 8, 16, 24, 32, 8, 16, 24, 32, 16, 32, 48, 64});

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
        gt(c, i, j, 0) = fetch::math::AsType<DataType>(
            gt_input[c * output_width * output_height + (i + j * output_width)]);
      }
    }
  }

  fetch::ml::ops::MaxPool2D<TensorType> op(3, 2);
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(MaxPoolTest, backward_test_2d)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;

  SizeType const input_width   = 5;
  SizeType const input_height  = 5;
  SizeType const output_width  = 2;
  SizeType const output_height = 2;
  SizeType const batch_size    = 2;

  TensorType data({1, input_width, input_height, batch_size});
  TensorType error({1, output_width, output_height, batch_size});
  TensorType gt({1, input_width, input_height, batch_size});

  for (SizeType i{0}; i < input_width; ++i)
  {
    for (SizeType j{0}; j < input_height; ++j)
    {
      data(0, i, j, 0) = static_cast<DataType>(i * j);
      gt(0, i, j, 0)   = DataType{0};
    }
  }

  for (SizeType i{0}; i < output_width; ++i)
  {
    for (SizeType j{0}; j < output_height; ++j)
    {
      error(0, i, j, 0) = static_cast<DataType>(1 + i + j);
    }
  }

  gt(0, 2, 2, 0) = DataType{1};
  gt(0, 4, 2, 0) = DataType{2};
  gt(0, 2, 4, 0) = DataType{2};
  gt(0, 4, 4, 0) = DataType{3};

  fetch::ml::ops::MaxPool2D<TensorType> op(3, 2);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(
      prediction.at(0).AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(MaxPoolTest, backward_2_channels_test_2d)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 5;
  SizeType const input_height  = 5;
  SizeType const output_width  = 2;
  SizeType const output_height = 2;
  SizeType const batch_size    = 2;

  TensorType data({channels_size, input_width, input_height, batch_size});
  TensorType error({channels_size, output_width, output_height, batch_size});
  TensorType gt({channels_size, input_width, input_height, batch_size});

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < input_width; ++i)
    {
      for (SizeType j{0}; j < input_height; ++j)
      {
        data(c, i, j, 0) = fetch::math::AsType<DataType>((c + 1) * i * j);
        gt(c, i, j, 0)   = DataType{0};
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

  gt(0, 2, 2, 0) = DataType{1};
  gt(0, 4, 2, 0) = DataType{2};
  gt(0, 2, 4, 0) = DataType{2};
  gt(0, 4, 4, 0) = DataType{3};
  gt(1, 2, 2, 0) = DataType{2};
  gt(1, 4, 2, 0) = DataType{4};
  gt(1, 2, 4, 0) = DataType{4};
  gt(1, 4, 4, 0) = DataType{6};

  fetch::ml::ops::MaxPool2D<TensorType> op(3, 2);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(
      prediction.at(0).AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(MaxPoolTest, saveparams_test_2d)
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

  SizeType const output_width  = 4;
  SizeType const output_height = 2;

  SizeType const batch_size = 2;

  TensorType          data({channels_size, input_width, input_height, batch_size});
  TensorType          gt({channels_size, output_width, output_height, batch_size});
  std::vector<double> gt_input({4, 8, 12, 16, 8, 16, 24, 32, 8, 16, 24, 32, 16, 32, 48, 64});

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
        gt(c, i, j, 0) = fetch::math::AsType<DataType>(
            gt_input[c * output_width * output_height + (i + j * output_width)]);
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

TYPED_TEST(MaxPoolTest, saveparams_backward_2_channels_test_2d)
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

}  // namespace test
}  // namespace ml
}  // namespace fetch
