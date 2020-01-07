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
#include "ml/ops/avg_pool_1d.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class AvgPool1DTest : public ::testing::Test
{
};

TYPED_TEST_CASE(AvgPool1DTest, math::test::TensorFloatingTypes);

TYPED_TEST(AvgPool1DTest, forward_test_3_2_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType data({1, 10, 2});
  TensorType gt({1, 4, 2});
  TensorType data_input = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8, 9, -10");
  TensorType gt_input({1, 4});

  gt_input(0, 0) = DataType{2} / DataType{3};
  gt_input(0, 1) = DataType{4} / DataType{3};
  gt_input(0, 2) = DataType{6} / DataType{3};
  gt_input(0, 3) = DataType{8} / DataType{3};

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 10; ++i)
    {
      data(0, i, i_b) =
          data_input[i] + fetch::math::AsType<DataType>(static_cast<double>(i_b * 10));
    }
    for (SizeType i{0}; i < 4; ++i)
    {
      gt(0, i, i_b) = gt_input[i] + fetch::math::AsType<DataType>(static_cast<double>(i_b * 10));
    }
  }

  fetch::ml::ops::AvgPool1D<TensorType> op(3, 2);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(AvgPool1DTest, backward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType data({1, 10, 2});
  TensorType error({1, 4, 2});
  TensorType gt({1, 10, 2});
  TensorType data_input  = TensorType::FromString("1, -2, 3, -4, 10, -6, 7, -8, 9, -10");
  TensorType error_input = TensorType::FromString("2, 3, 4, 5");

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 10; ++i)
    {
      data(0, i, i_b) = data_input[i] + static_cast<DataType>(i_b);
    }
    for (SizeType i{0}; i < 4; ++i)
    {
      error(0, i, i_b) = error_input[i] + static_cast<DataType>(i_b);
    }
  }

  gt(0, 0, 0) = DataType{2} / DataType{3};
  gt(0, 0, 1) = DataType{1};
  gt(0, 1, 0) = DataType{2} / DataType{3};
  gt(0, 1, 1) = DataType{1};
  gt(0, 2, 0) = DataType{5} / DataType{3};
  gt(0, 2, 1) = DataType{7} / DataType{3};
  gt(0, 3, 0) = DataType{1};
  gt(0, 3, 1) = DataType{4} / DataType{3};
  gt(0, 4, 0) = DataType{7} / DataType{3};
  gt(0, 4, 1) = DataType{3};
  gt(0, 5, 0) = DataType{4} / DataType{3};
  gt(0, 5, 1) = DataType{5} / DataType{3};
  gt(0, 6, 0) = DataType{3};
  gt(0, 6, 1) = DataType{11} / DataType{3};
  gt(0, 7, 0) = DataType{5} / DataType{3};
  gt(0, 7, 1) = DataType{2};
  gt(0, 8, 0) = DataType{5} / DataType{3};
  gt(0, 8, 1) = DataType{2};
  gt(0, 9, 0) = DataType{0};
  gt(0, 9, 1) = DataType{0};

  fetch::ml::ops::AvgPool1D<TensorType> op(3, 2);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt,
                                     fetch::math::function_tolerance<typename TypeParam::Type>(),
                                     fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(AvgPool1DTest, backward_test_2_channels)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType data({2, 5, 2});
  TensorType error({2, 2, 2});
  TensorType gt({2, 5, 2});
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

  gt(0, 0, 0) = DataType{1} / DataType{2};
  gt(0, 0, 1) = DataType{0};
  gt(0, 1, 0) = DataType{5} / DataType{4};
  gt(0, 1, 1) = DataType{0};
  gt(0, 2, 0) = DataType{5} / DataType{4};
  gt(0, 2, 1) = DataType{0};
  gt(0, 3, 0) = DataType{5} / DataType{4};
  gt(0, 3, 1) = DataType{0};
  gt(0, 4, 0) = DataType{3} / DataType{4};
  gt(0, 4, 1) = DataType{0};
  gt(1, 0, 0) = DataType{1};
  gt(1, 0, 1) = DataType{0};
  gt(1, 1, 0) = DataType{9} / DataType{4};
  gt(1, 1, 1) = DataType{0};
  gt(1, 2, 0) = DataType{9} / DataType{4};
  gt(1, 2, 1) = DataType{0};
  gt(1, 3, 0) = DataType{9} / DataType{4};
  gt(1, 3, 1) = DataType{0};
  gt(1, 4, 0) = DataType{5} / DataType{4};
  gt(1, 4, 1) = DataType{0};

  fetch::ml::ops::AvgPool1D<TensorType> op(4, 1);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt,
                                     fetch::math::function_tolerance<typename TypeParam::Type>(),
                                     fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(AvgPool1DTest, forward_test_4_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType data({1, 10, 1});
  TensorType gt({1, 4, 1});
  TensorType data_input = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8, 9, -10");
  TensorType gt_input   = TensorType::FromString("-0.5, -0.5, -0.5, -0.5");
  for (SizeType i{0}; i < 10; ++i)
  {
    data(0, i, 0) = data_input[i];
  }

  for (SizeType i{0}; i < 4; ++i)
  {
    gt(0, i, 0) = gt_input[i];
  }

  fetch::ml::ops::AvgPool1D<TensorType> op(4, 2);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(AvgPool1DTest, forward_test_2_channels_4_1_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType data({2, 5, 2});
  TensorType gt({2, 2, 2});
  TensorType data_input = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8, 9, -10");

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 5; ++j)
      {
        data(i, j, i_b) =
            data_input[i * 5 + j] + fetch::math::AsType<DataType>(static_cast<double>(i_b * 10));
      }
    }
  }

  gt(0, 0, 0) = DataType{-1} / DataType{2};
  gt(0, 0, 1) = DataType{19} / DataType{2};
  gt(0, 1, 0) = DataType{1} / DataType{2};
  gt(0, 1, 1) = DataType{21} / DataType{2};
  gt(1, 0, 0) = DataType{1} / DataType{2};
  gt(1, 0, 1) = DataType{21} / DataType{2};
  gt(1, 1, 0) = DataType{-1} / DataType{2};
  gt(1, 1, 1) = DataType{19} / DataType{2};

  fetch::ml::ops::AvgPool1D<TensorType> op(4, 1);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(AvgPool1DTest, forward_test_2_4_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType data({1, 10, 2});
  TensorType gt({1, 3, 2});
  TensorType data_input = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8, 9, -10");
  TensorType gt_input   = TensorType::FromString("-0.5, -0.5, -0.5");
  for (SizeType i{0}; i < 10; ++i)
  {
    data(0, i, 0) = data_input[i];
  }

  for (SizeType i{0}; i < 3; ++i)
  {
    gt(0, i, 0) = gt_input[i];
  }

  fetch::ml::ops::AvgPool1D<TensorType> op(2, 4);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(AvgPool1DTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::AvgPool1D<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::AvgPool1D<TensorType>;
  using SizeType      = fetch::math::SizeType;

  TensorType data({2, 5, 2});
  TensorType gt({2, 2, 2});
  TensorType data_input = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8, 9, -10");
  TensorType gt_input   = TensorType::FromString("3, 5, 9, 9");

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 5; ++j)
      {
        data(i, j, i_b) =
            data_input[i * 5 + j] + fetch::math::AsType<DataType>(static_cast<double>(i_b * 10));
      }
    }
  }

  gt(0, 0, 0) = DataType{-1} / DataType{2};
  gt(0, 0, 1) = DataType{19} / DataType{2};
  gt(0, 1, 0) = DataType{1} / DataType{2};
  gt(0, 1, 1) = DataType{21} / DataType{2};
  gt(1, 0, 0) = DataType{1} / DataType{2};
  gt(1, 0, 1) = DataType{21} / DataType{2};
  gt(1, 1, 0) = DataType{-1} / DataType{2};
  gt(1, 1, 1) = DataType{19} / DataType{2};

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

TYPED_TEST(AvgPool1DTest, saveparams_backward_test_2_channels)
{
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;
  using OpType     = typename fetch::ml::ops::AvgPool1D<TensorType>;
  using SPType     = typename OpType::SPType;

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
