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
#include "ml/ops/one_hot.hpp"
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
class OneHotOpTest : public ::testing::Test
{
};

TYPED_TEST_CASE(OneHotOpTest, math::test::TensorFloatingTypes);

TYPED_TEST(OneHotOpTest, forward_test)
{
  using DataType  = typename TypeParam::Type;
  using SizeType  = typename TypeParam::SizeType;
  using ArrayType = TypeParam;

  ArrayType data = TypeParam::FromString("1,0,1,2");
  data.Reshape({2, 2, 1, 1});
  ArrayType gt = TypeParam::FromString("-1, 5, -1; 5, -1, -1; -1, 5, -1; -1, -1, 5");
  gt.Reshape({2, 2, 1, 3, 1});

  SizeType depth     = 3;
  SizeType axis      = 3;
  auto     on_value  = DataType{5};
  auto     off_value = DataType{-1};

  fetch::ml::ops::OneHot<TypeParam> op(depth, axis, on_value, off_value);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(data)}));
  op.Forward({std::make_shared<TypeParam>(data)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(OneHotOpTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using SizeType      = typename TypeParam::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::OneHot<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::OneHot<TensorType>;

  TensorType data = TypeParam::FromString("1,0,1,2");
  data.Reshape({2, 2, 1, 1});
  TensorType gt = TypeParam::FromString("-1, 5, -1; 5, -1, -1; -1, 5, -1; -1, -1, 5");
  gt.Reshape({2, 2, 1, 3, 1});

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

}  // namespace test
}  // namespace ml
}  // namespace fetch
