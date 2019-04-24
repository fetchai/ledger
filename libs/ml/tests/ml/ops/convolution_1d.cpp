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

#include "ml/ops/convolution_1d.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class Convolution1DTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(Convolution1DTest, MyTypes);

TYPED_TEST(Convolution1DTest, forward_1x1_1x1x1)
{
  TypeParam input(std::vector<uint64_t>({1, 1}));
  TypeParam weigths(std::vector<uint64_t>({1, 1, 1}));
  input.At(0, 0)      = typename TypeParam::Type(5);
  weigths.At(0, 0, 0) = typename TypeParam::Type(-4);
  fetch::ml::ops::Convolution1D<TypeParam> c;
  TypeParam                                output = c.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({input, weigths}));

  ASSERT_EQ(output.shape(), std::vector<typename TypeParam::SizeType>({1, 1}));
  EXPECT_EQ(output.At(0, 0), typename TypeParam::Type(-20));
}

TYPED_TEST(Convolution1DTest, forward_1x3_1x1x3)
{
  using SizeType = typename TypeParam::SizeType;
  TypeParam input(std::vector<uint64_t>({1, 3}));
  TypeParam weigths(std::vector<uint64_t>({1, 1, 3}));
  for (SizeType i(0); i < 3; ++i)
  {

    input.At(0, i)      = typename TypeParam::Type(i);
    weigths.At(0, 0, i) = typename TypeParam::Type(i);
  }
  fetch::ml::ops::Convolution1D<TypeParam> c;
  TypeParam                                output = c.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({input, weigths}));

  ASSERT_EQ(output.shape(), std::vector<uint64_t>({1, 1}));
  EXPECT_EQ(output.At(0, 0), typename TypeParam::Type(5));
}

TYPED_TEST(Convolution1DTest, forward_3x3_1x3x3)
{
  TypeParam     input(std::vector<uint64_t>({3, 3}));
  TypeParam     weigths(std::vector<uint64_t>({1, 3, 3}));
  std::uint64_t counter = 0;
  for (std::uint64_t i(0); i < 3; ++i)
  {

    for (std::uint64_t k(0); k < 3; ++k)
    {
      input.At(i, k)      = typename TypeParam::Type(counter);
      weigths.At(0, i, k) = typename TypeParam::Type(counter);
      ++counter;
    }
  }
  fetch::ml::ops::Convolution1D<TypeParam> c;
  TypeParam                                output = c.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({input, weigths}));

  ASSERT_EQ(output.shape(), std::vector<uint64_t>({1, 1}));
  EXPECT_EQ(output.At(0, 0), typename TypeParam::Type(204));
}

TYPED_TEST(Convolution1DTest, forward_3x3_5x3x3)
{
  TypeParam                                input(std::vector<uint64_t>({3, 3}));
  TypeParam                                weigths(std::vector<uint64_t>({5, 3, 3}));
  fetch::ml::ops::Convolution1D<TypeParam> c;
  TypeParam                                output = c.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({input, weigths}));

  ASSERT_EQ(output.shape(), std::vector<uint64_t>({5, 1}));
}

TYPED_TEST(Convolution1DTest, forward_1x5_1x1x3)
{
  TypeParam                                input(std::vector<uint64_t>({1, 5}));
  TypeParam                                weigths(std::vector<uint64_t>({1, 1, 3}));
  fetch::ml::ops::Convolution1D<TypeParam> c;
  TypeParam                                output = c.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({input, weigths}));

  ASSERT_EQ(output.shape(), std::vector<uint64_t>({1, 3}));
}
