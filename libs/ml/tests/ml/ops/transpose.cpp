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

#include "ml/ops/transpose.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class TransposeTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(TransposeTest, MyTypes);

TYPED_TEST(TransposeTest, forward_non_batch)
{
  std::shared_ptr<TypeParam> input =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 8}));
  fetch::ml::ops::Transpose<TypeParam> op;
  typename TypeParam::Type             i(0);
  for (auto &e : *input)
  {
    e = i;
    i += typename TypeParam::Type(1);
  }
  std::shared_ptr<TypeParam> output = op.Forward({input});

  EXPECT_EQ(input->shape(), std::vector<uint64_t>({5, 8}));
  EXPECT_EQ(output->shape(), std::vector<uint64_t>({8, 5}));
  for (uint64_t y(0); y < 5; ++y)
  {
    for (uint64_t x(0); x < 8; ++x)
    {
      // Check that input didn't got modified
      EXPECT_EQ(input->At({y, x}), typename TypeParam::Type(y * 8 + x));
      // Check that output is transpose of input
      EXPECT_EQ(input->At({y, x}), output->At({x, y}));
    }
  }
}

TYPED_TEST(TransposeTest, forward_batch)
{
  std::shared_ptr<TypeParam> input =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({3, 5, 8}));
  fetch::ml::ops::Transpose<TypeParam> op;
  typename TypeParam::Type             i(0);
  for (auto &e : *input)
  {
    e = i;
    i += typename TypeParam::Type(1);
  }
  std::shared_ptr<TypeParam> output = op.Forward({input});

  EXPECT_EQ(input->shape(), std::vector<uint64_t>({3, 5, 8}));
  EXPECT_EQ(output->shape(), std::vector<uint64_t>({3, 8, 5}));
  for (uint64_t b(0); b < 3; ++b)
  {
    for (uint64_t y(0); y < 5; ++y)
    {
      for (uint64_t x(0); x < 8; ++x)
      {
        // Check that input didn't got modified
        EXPECT_EQ(input->At({b, y, x}), typename TypeParam::Type(b * 40 + y * 8 + x));
        // Check that output is transpose of input
        EXPECT_EQ(input->At({b, y, x}), output->At({b, x, y}));
      }
    }
  }
}
