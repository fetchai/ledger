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

#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>
#include "ml/ops/tanh.hpp"


template <typename T>
class TanhTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>,
                                 fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>
                                 >;

TYPED_TEST_CASE(TanhTest, MyTypes);

TYPED_TEST(TanhTest, forward_all_positive_test)
{
  TypeParam     data(1);
  TypeParam     gt(10);

  std::vector<float> dataInput({0, 0.2, 0.4, 0.6, 0.8, 1, 1.2, 1.4, 20, 100});
  std::vector<float> gtInput({0.0, 0.197375, 0.379949, 0.53705, 0.664037, 0.761594, 0.833655, 0.885352, 1.0, 1.0});
  std::uint64_t i(0);

  for (std::uint64_t i(0); i < 10; ++i)
  {
    data.Set(i, typename TypeParam::Type(dataInput[i]));
    gt.Set(i, typename TypeParam::Type(gtInput[i]));
    i++;
  }

  fetch::ml::ops::Tanh<TypeParam> op;
  TypeParam                       prediction = op.Forward({data});

  ASSERT_TRUE(prediction.AllClose(gt));
}


TYPED_TEST(TanhTest, forward_all_negative_test)
{
  TypeParam     data(1);
  TypeParam     gt(10);

  std::vector<float> dataInput({-0, -0.2, -0.4, -0.6, -0.8, -1, -1.2, -1.4, -20, -100});
  std::vector<float> gtInput({-0.0, -0.197375, -0.379949, -0.53705, -0.664037, -0.761594, -0.833655, -0.885352, -1.0, -1.0});
  std::uint64_t i(0);

  for (std::uint64_t i(0); i < 10; ++i)
  {
    data.Set(i, typename TypeParam::Type(dataInput[i]));
    gt.Set(i, typename TypeParam::Type(gtInput[i]));
    i++;
  }

  fetch::ml::ops::Tanh<TypeParam> op;
  TypeParam                       prediction = op.Forward({data});

  ASSERT_TRUE(prediction.AllClose(gt));
}
