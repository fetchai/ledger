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

#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "ml/ops/abs.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <vector>
#include <ml/ops/boolean_mask.hpp>

template <typename T>
class BooleanMaskTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                                 fetch::math::Tensor<fetch::fixed_point::fp64_t>>;

TYPED_TEST_CASE(BooleanMaskTest, MyTypes);

TYPED_TEST(BooleanMaskTest, forward_test)
{
  using ArrayType = TypeParam;
  using DataType  = typename TypeParam::Type;

  ArrayType mask = ArrayType::FromString(
      "1, 0, 1, 0, 0, 0, 0, 1, 1");
  mask.Reshape({3, 3, 1});
	
	ArrayType target_input = ArrayType::FromString(
	 "3, 6, 2, 1, 3, -2, 2, 1, -9");
	target_input.Reshape({3, 3, 1});
	
	ArrayType mask_value({3, 3, 1});
	mask_value.Fill(static_cast<DataType>(-100));

  ArrayType gt = ArrayType::FromString(
      "3, -100, 2, -100, -100, -100, -100, 1, -9");
  gt.Reshape({3, 3, 1});

  fetch::ml::ops::BooleanMask<ArrayType> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const ArrayType>(mask), std::make_shared<const ArrayType>(target_input), std::make_shared<const ArrayType>(mask_value)}));
  op.Forward({std::make_shared<const ArrayType>(mask), std::make_shared<const ArrayType>(target_input), std::make_shared<const ArrayType>(mask_value)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(), fetch::math::function_tolerance<DataType>()));
}

//TYPED_TEST(BooleanMaskTest, backward_test)
//{
//  using ArrayType = TypeParam;
//  using DataType  = typename TypeParam::Type;
//
//  ArrayType data = ArrayType::FromString(
//      "1, -2, 3,-4, 5,-6, 7,-8;"
//      "1,  2, 3, 4, 5, 6, 7, 8");
//
//  ArrayType gt = ArrayType::FromString(
//      "1, 1, 2, 2, 3, 3, 4, 4;"
//      "5, -5, 6, -6, 7, -7, 8, -8");
//
//  ArrayType error = ArrayType::FromString(
//      "1, -1, 2, -2, 3, -3, 4, -4;"
//      "5, -5, 6, -6, 7, -7, 8, -8");
//
//  fetch::ml::ops::Abs<ArrayType> op;
//  std::vector<ArrayType> prediction = op.Backward({std::make_shared<const ArrayType>(data)}, error);
//
//  // test correct values
//  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>(),
//                                     fetch::math::function_tolerance<DataType>()));
//}
