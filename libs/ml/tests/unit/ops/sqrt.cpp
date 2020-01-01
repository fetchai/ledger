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
#include "ml/ops/sqrt.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class SqrtTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SqrtTest, math::test::TensorFloatingTypes);

TYPED_TEST(SqrtTest, forward_all_positive_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  TensorType data = TensorType::FromString("0, 1, 2, 4, 10, 100");
  TensorType gt   = TensorType::FromString("0, 1, 1.41421356, 2, 3.1622776, 10");

  fetch::ml::ops::Sqrt<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SqrtTest, backward_all_positive_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  TensorType data  = TensorType::FromString("1,   2,         4,   10,       100");
  TensorType error = TensorType::FromString("1,   1,         1,    2,         0");
  // 0.5 / sqrt(data) * error = gt
  TensorType gt = TensorType::FromString("0.5, 0.3535533, 0.25, 0.3162277, 0");

  fetch::ml::ops::Sqrt<TypeParam> op;

  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  ASSERT_TRUE(prediction.at(0).AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                        fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SqrtTest, forward_all_negative_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  TensorType data = TensorType::FromString("-1, -2, -4, -10, -100");

  fetch::ml::ops::Sqrt<TypeParam> op;

  TypeParam pred(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, pred);

  // gives NaN because sqrt of a negative number is undefined
  for (auto const &p_it : pred)
  {
    EXPECT_TRUE(fetch::math::is_nan(p_it));
  }
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SqrtTest, backward_all_negative_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data  = TensorType::FromString("-1, -2, -4, -10, -100");
  TensorType error = TensorType::FromString("1,   1,  1,   2,    0");

  fetch::ml::ops::Sqrt<TypeParam> op;

  std::vector<TensorType> pred = op.Backward({std::make_shared<const TensorType>(data)}, error);
  // gives NaN because sqrt of a negative number is undefined
  for (auto const &p_it : pred.at(0))
  {
    EXPECT_TRUE(fetch::math::is_nan(p_it));
  }
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SqrtTest, backward_zero_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data  = TensorType::FromString("0,  0,    0,    0,        0");
  TensorType error = TensorType::FromString("1,  1,    1,    2,        0");

  fetch::ml::ops::Sqrt<TypeParam> op;

  std::vector<TensorType> pred = op.Backward({std::make_shared<const TensorType>(data)}, error);
  // gives NaN because of division by zero
  for (auto const &p_it : pred.at(0))
  {
    EXPECT_TRUE(fetch::math::is_inf(p_it) || fetch::math::is_nan(p_it));
  }
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SqrtTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Sqrt<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::Sqrt<TensorType>;

  TensorType data = TensorType::FromString("0, 1, 2, 4, 10, 100");
  TensorType gt   = TensorType::FromString("0, 1, 1.41421356, 2, 3.1622776, 10");

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

TYPED_TEST(SqrtTest, saveparams_backward_all_positive_test)
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

}  // namespace test
}  // namespace ml
}  // namespace fetch
