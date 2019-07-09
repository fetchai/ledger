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

#include "ml/ops/divide.hpp"

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

namespace {
template <typename T>
class DivideTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(DivideTest, MyTypes);

TYPED_TEST(DivideTest, forward_test)
{
  TypeParam data  = TypeParam::FromString("1, 1, 2; 3, 4, 5");
  TypeParam data2 = TypeParam::FromString("1, 1, 2; 3, 4, 5");
  TypeParam gt    = TypeParam::FromString("1, 1, 1; 1, 1, 1");

  fetch::ml::ops::Divide<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({data}));
  op.Forward({data, data2}, prediction);

  EXPECT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));

  data  = TypeParam::FromString("0, 1, 2; 3, 4, 5");
  data2 = TypeParam::FromString("1, -2, 3; -4, 5, -6");
  gt    = TypeParam::FromString("0, -0.5, 0.666666666; -0.75, 0.8, -0.8333333");

  op.Forward({data, data2}, prediction);

  EXPECT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(DivideTest, backward_test)
{
  TypeParam data = TypeParam::FromString("0, 1, 2; 3, 4, 5");

  fetch::ml::ops::Divide<TypeParam> op;

  TypeParam              error(op.ComputeOutputShape({data}));
  std::vector<TypeParam> ret = op.Backward({data, data}, error);

  for (auto &pt : ret)
  {
    EXPECT_TRUE(error.AllClose(pt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                               fetch::math::function_tolerance<typename TypeParam::Type>()));
  }
}
}  // namespace
