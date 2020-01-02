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
#include "ml/ops/avg_pool_2d.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class AvgPool2DTest : public ::testing::Test
{
};

TYPED_TEST_CASE(AvgPool2DTest, math::test::TensorFloatingTypes);

TYPED_TEST(AvgPool2DTest, forward_test_3_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_width  = 10;
  SizeType const input_height = 5;

  SizeType const output_width  = 4;
  SizeType const output_height = 2;

  SizeType const batch_size = 2;

  TensorType data({1, input_width, input_height, batch_size});
  TensorType gt({1, output_width, output_height, batch_size});
  TensorType gt_input = TensorType::FromString("1, 3, 5, 7, 3, 9, 15, 21");

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
      gt(0, i, j, 0) = gt_input[i + j * output_width];
    }
  }

  fetch::ml::ops::AvgPool2D<TensorType> op(3, 2);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(AvgPool2DTest, forward_2_channels_test_3_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 10;
  SizeType const input_height  = 5;

  SizeType const output_width  = 4;
  SizeType const output_height = 2;

  SizeType const batch_size = 2;

  TensorType data({channels_size, input_width, input_height, batch_size});
  TensorType gt({channels_size, output_width, output_height, batch_size});
  TensorType gt_input =
      TensorType::FromString("1, 3, 5, 7, 3, 9, 15, 21, 2, 6, 10, 14, 6, 18, 30, 42");

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
        gt(c, i, j, 0) = gt_input[c * output_width * output_height + (i + j * output_width)];
      }
    }
  }

  fetch::ml::ops::AvgPool2D<TensorType> op(3, 2);
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(AvgPool2DTest, backward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

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
      data(0, i, j, 0) = fetch::math::AsType<DataType>(i * j);
      gt(0, i, j, 0)   = DataType{0};
    }
  }

  for (SizeType i{0}; i < output_width; ++i)
  {
    for (SizeType j{0}; j < output_height; ++j)
    {
      error(0, i, j, 0) = fetch::math::AsType<DataType>(1 + i + j);
    }
  }

  gt(0, 0, 0, 0) = DataType{1} / DataType{9};
  gt(0, 0, 1, 0) = DataType{1} / DataType{9};
  gt(0, 0, 2, 0) = DataType{3} / DataType{9};
  gt(0, 0, 3, 0) = DataType{2} / DataType{9};
  gt(0, 0, 4, 0) = DataType{2} / DataType{9};
  gt(0, 1, 0, 0) = DataType{1} / DataType{9};
  gt(0, 1, 1, 0) = DataType{1} / DataType{9};
  gt(0, 1, 2, 0) = DataType{3} / DataType{9};
  gt(0, 1, 3, 0) = DataType{2} / DataType{9};
  gt(0, 1, 4, 0) = DataType{2} / DataType{9};
  gt(0, 2, 0, 0) = DataType{3} / DataType{9};
  gt(0, 2, 1, 0) = DataType{3} / DataType{9};
  gt(0, 2, 2, 0) = DataType{8} / DataType{9};
  gt(0, 2, 3, 0) = DataType{5} / DataType{9};
  gt(0, 2, 4, 0) = DataType{5} / DataType{9};
  gt(0, 3, 0, 0) = DataType{2} / DataType{9};
  gt(0, 3, 1, 0) = DataType{2} / DataType{9};
  gt(0, 3, 2, 0) = DataType{5} / DataType{9};
  gt(0, 3, 3, 0) = DataType{3} / DataType{9};
  gt(0, 3, 4, 0) = DataType{3} / DataType{9};
  gt(0, 4, 0, 0) = DataType{2} / DataType{9};
  gt(0, 4, 1, 0) = DataType{2} / DataType{9};
  gt(0, 4, 2, 0) = DataType{5} / DataType{9};
  gt(0, 4, 3, 0) = DataType{3} / DataType{9};
  gt(0, 4, 4, 0) = DataType{3} / DataType{9};

  fetch::ml::ops::AvgPool2D<TensorType> op(3, 2);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt,
                                     fetch::math::function_tolerance<typename TypeParam::Type>(),
                                     fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(AvgPool2DTest, backward_2_channels_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

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

  gt(0, 0, 0, 0) = DataType{1} / DataType{9};
  gt(0, 0, 1, 0) = DataType{1} / DataType{9};
  gt(0, 0, 2, 0) = DataType{3} / DataType{9};
  gt(0, 0, 3, 0) = DataType{2} / DataType{9};
  gt(0, 0, 4, 0) = DataType{2} / DataType{9};
  gt(0, 1, 0, 0) = DataType{1} / DataType{9};
  gt(0, 1, 1, 0) = DataType{1} / DataType{9};
  gt(0, 1, 2, 0) = DataType{3} / DataType{9};
  gt(0, 1, 3, 0) = DataType{2} / DataType{9};
  gt(0, 1, 4, 0) = DataType{2} / DataType{9};
  gt(0, 2, 0, 0) = DataType{3} / DataType{9};
  gt(0, 2, 1, 0) = DataType{3} / DataType{9};
  gt(0, 2, 2, 0) = DataType{8} / DataType{9};
  gt(0, 2, 3, 0) = DataType{5} / DataType{9};
  gt(0, 2, 4, 0) = DataType{5} / DataType{9};
  gt(0, 3, 0, 0) = DataType{2} / DataType{9};
  gt(0, 3, 1, 0) = DataType{2} / DataType{9};
  gt(0, 3, 2, 0) = DataType{5} / DataType{9};
  gt(0, 3, 3, 0) = DataType{3} / DataType{9};
  gt(0, 3, 4, 0) = DataType{3} / DataType{9};
  gt(0, 4, 0, 0) = DataType{2} / DataType{9};
  gt(0, 4, 1, 0) = DataType{2} / DataType{9};
  gt(0, 4, 2, 0) = DataType{5} / DataType{9};
  gt(0, 4, 3, 0) = DataType{3} / DataType{9};
  gt(0, 4, 4, 0) = DataType{3} / DataType{9};
  gt(1, 0, 0, 0) = DataType{2} / DataType{9};
  gt(1, 0, 1, 0) = DataType{2} / DataType{9};
  gt(1, 0, 2, 0) = DataType{6} / DataType{9};
  gt(1, 0, 3, 0) = DataType{4} / DataType{9};
  gt(1, 0, 4, 0) = DataType{4} / DataType{9};
  gt(1, 1, 0, 0) = DataType{2} / DataType{9};
  gt(1, 1, 1, 0) = DataType{2} / DataType{9};
  gt(1, 1, 2, 0) = DataType{6} / DataType{9};
  gt(1, 1, 3, 0) = DataType{4} / DataType{9};
  gt(1, 1, 4, 0) = DataType{4} / DataType{9};
  gt(1, 2, 0, 0) = DataType{6} / DataType{9};
  gt(1, 2, 1, 0) = DataType{6} / DataType{9};
  gt(1, 2, 2, 0) = DataType{16} / DataType{9};
  gt(1, 2, 3, 0) = DataType{10} / DataType{9};
  gt(1, 2, 4, 0) = DataType{10} / DataType{9};
  gt(1, 3, 0, 0) = DataType{4} / DataType{9};
  gt(1, 3, 1, 0) = DataType{4} / DataType{9};
  gt(1, 3, 2, 0) = DataType{10} / DataType{9};
  gt(1, 3, 3, 0) = DataType{6} / DataType{9};
  gt(1, 3, 4, 0) = DataType{6} / DataType{9};
  gt(1, 4, 0, 0) = DataType{4} / DataType{9};
  gt(1, 4, 1, 0) = DataType{4} / DataType{9};
  gt(1, 4, 2, 0) = DataType{10} / DataType{9};
  gt(1, 4, 3, 0) = DataType{6} / DataType{9};
  gt(1, 4, 4, 0) = DataType{6} / DataType{9};

  fetch::ml::ops::AvgPool2D<TensorType> op(3, 2);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt,
                                     fetch::math::function_tolerance<typename TypeParam::Type>(),
                                     fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(AvgPool2DTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::AvgPool2D<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::AvgPool2D<TensorType>;
  using SizeType      = fetch::math::SizeType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 10;
  SizeType const input_height  = 5;

  SizeType const output_width  = 4;
  SizeType const output_height = 2;

  SizeType const batch_size = 2;

  TensorType data({channels_size, input_width, input_height, batch_size});
  TensorType gt({channels_size, output_width, output_height, batch_size});
  TensorType gt_input =
      TensorType::FromString("1, 3, 5, 7, 3, 9, 15, 21, 2, 6, 10, 14, 6, 18, 30, 42");

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
        gt(c, i, j, 0) = gt_input[c * output_width * output_height + (i + j * output_width)];
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

TYPED_TEST(AvgPool2DTest, saveparams_backward_2_channels_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;
  using OpType     = typename fetch::ml::ops::AvgPool2D<TensorType>;
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

  fetch::ml::ops::AvgPool2D<TensorType> op(3, 2);
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
