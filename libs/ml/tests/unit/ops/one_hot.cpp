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

#include "test_types.hpp"

#include "ml/ops/one_hot.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using namespace fetch::ml;

template <typename T>
class OneHotOpTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(OneHotOpTest, fetch::math::test::TensorFloatingTypes, );

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

  TypeParam prediction(op.ComputeOutputShape({data.shape()}));
  op.Forward({std::make_shared<TypeParam>(data)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

}  // namespace
