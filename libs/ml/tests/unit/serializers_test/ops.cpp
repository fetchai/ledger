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

#include "ml/serializers/ops/abs.hpp"

namespace {

template <typename T>
class SerialiaseOpsTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SerialiaseOpsTest, ::fetch::math::test::TensorFloatingTypes);

///////////
/// OPS ///
///////////

///////////
/// ABS ///
///////////

TYPED_TEST(SerialiaseOpsTest, abs_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
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

  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<OpKind, TypeParam>(op);
  OpKind new_op(*dsp);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}
//
// TYPED_TEST(SerialiaseOpsTest, abs_saveparams_backward_test)
//{
//  using TensorType = TypeParam;
//  using OpKind     = fetch::ml::ops::Abs<TensorType>;
//  using SPType     = typename OpKind::SPType;
//
//  TensorType data = TensorType::FromString(
//      "1, -2, 3,-4, 5,-6, 7,-8;"
//      "1,  2, 3, 4, 5, 6, 7, 8");
//
//  TensorType error = TensorType::FromString(
//      "1, -1, 2, -2, 3, -3, 4, -4;"
//      "5, -5, 6, -6, 7, -7, 8, -8");
//
//  fetch::ml::ops::Abs<TensorType> op;
//
//  // run op once to make sure caches etc. have been filled. Otherwise the test might be trivial!
//  std::vector<TensorType> prediction =
//      op.Backward({std::make_shared<const TensorType>(data)}, error);
//
//  auto   dsp = serializer_test_utils::SerialiseDeserialiseBuild<SPType, TypeParam>(op);
//  OpKind new_op(*dsp);
//
//  // check that new predictions match the old
//  std::vector<TensorType> new_prediction =
//      new_op.Backward({std::make_shared<const TensorType>(data)}, error);
//
//  // test correct values
//  EXPECT_TRUE(prediction.at(0).AllClose(
//      new_prediction.at(0), ::fetch::math::function_tolerance<typename TypeParam::Type>(),
//      ::fetch::math::function_tolerance<typename TypeParam::Type>()));
//}

}  // namespace
