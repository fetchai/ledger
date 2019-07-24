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

#include "ml/ops/log.hpp"

#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vectorise/fixed_point/serializers.hpp"

#include "gtest/gtest.h"

#include <cmath>
#include <cstdint>
#include <vector>

template <typename T>
class LogFloatTest : public ::testing::Test
{
};

template <typename T>
class LogFixedTest : public ::testing::Test
{
};

template <typename T>
class LogBothTest : public ::testing::Test
{
};

using FloatingPointTypes =
    ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>>;

using FixedPointTypes = ::testing::Types<fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                                         fetch::math::Tensor<fetch::fixed_point::fp64_t>>;

using BothTypes = ::testing::Types<fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                                   fetch::math::Tensor<fetch::fixed_point::fp64_t>,
                                   fetch::math::Tensor<float>, fetch::math::Tensor<double>>;

TYPED_TEST_CASE(LogFloatTest, FloatingPointTypes);
TYPED_TEST_CASE(LogFixedTest, FixedPointTypes);
TYPED_TEST_CASE(LogBothTest, BothTypes);

TYPED_TEST(LogBothTest, forward_all_positive_test)
{
  using ArrayType = TypeParam;
  using DataType  = typename TypeParam::Type;

  ArrayType data = ArrayType::FromString("1, 2, 4, 8, 100, 1000");
  ArrayType gt   = ArrayType::FromString(
      "0, 0.693147180559945, 1.38629436111989, 2.07944154167984, 4.60517018598809, "
      "6.90775527898214");

  fetch::ml::ops::Log<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

// TODO(1195): fixed point and floating point tests should be unified.
TYPED_TEST(LogFloatTest, forward_all_negative_test)
{
  using ArrayType = TypeParam;

  ArrayType data = ArrayType::FromString("-1, -2, -4, -10, -100");

  fetch::ml::ops::Log<TypeParam> op;

  TypeParam pred(op.ComputeOutputShape({data}));
  op.Forward({data}, pred);

  // gives NaN because log of a negative number is undefined
  for (auto p_it : pred)
  {
    EXPECT_TRUE(std::isnan(p_it));
  }
}

TYPED_TEST(LogFixedTest, forward_all_negative_test)
{
  using ArrayType = TypeParam;

  ArrayType data = ArrayType::FromString("-1, -2, -4, -10, -100");

  fetch::ml::ops::Log<TypeParam> op;

  TypeParam pred(op.ComputeOutputShape({data}));
  op.Forward({data}, pred);

  // gives NaN because log of a negative number is undefined
  for (auto p_it : pred)
  {
    EXPECT_TRUE(ArrayType::Type::IsNaN(p_it));
  }
}

TYPED_TEST(LogBothTest, backward_test)
{
  using ArrayType = TypeParam;
  using DataType  = typename TypeParam::Type;

  ArrayType data  = ArrayType::FromString("1, -2, 4, -10, 100");
  ArrayType error = ArrayType::FromString("1, 1, 1, 2, 0");
  // (1 / data) * error = gt
  ArrayType gt = ArrayType::FromString("1,-0.5,0.25,-0.2,0");

  fetch::ml::ops::Log<TypeParam> op;

  std::vector<ArrayType> prediction = op.Backward({data}, error);

  ASSERT_TRUE(prediction.at(0).AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                        fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(LogBothTest, saveparams_test)
{
  using ArrayType     = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::Ops<ArrayType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Log<ArrayType>::SPType;
  using OpType        = typename fetch::ml::ops::Log<ArrayType>;

  ArrayType data = ArrayType::FromString("1, 2, 4, 8, 100, 1000");
  ArrayType gt   = ArrayType::FromString(
      "0, 0.693147180559945, 1.38629436111989, 2.07944154167984, 4.60517018598809, "
      "6.90775527898214");

  OpType op;

  ArrayType     prediction(op.ComputeOutputShape({data}));
  VecTensorType vec_data({data});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::SaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::ByteArrayBuffer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  ArrayType new_prediction(op.ComputeOutputShape({data}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, fetch::math::function_tolerance<DataType>(),
                                      fetch::math::function_tolerance<DataType>()));
}
