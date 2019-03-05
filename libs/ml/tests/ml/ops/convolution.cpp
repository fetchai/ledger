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

#include "ml/ops/convolution.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class ConvolutionTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>// , fetch::math::Tensor<float>,
                                 // fetch::math::Tensor<double>,
                                 // fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 // fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>
				 >;
TYPED_TEST_CASE(ConvolutionTest, MyTypes);

TYPED_TEST(ConvolutionTest, forward_1x1x1_1x1x1x1)
{
  std::shared_ptr<TypeParam> input = std::make_shared<TypeParam>(std::vector<uint64_t>({1, 1, 1}));
  std::shared_ptr<TypeParam> weigths = std::make_shared<TypeParam>(std::vector<uint64_t>({1, 1, 1, 1}));
  input->At(0) = typename TypeParam::Type(5);
  weigths->At(0) = typename TypeParam::Type(-4);
  fetch::ml::ops::Convolution<TypeParam> c;
  std::shared_ptr<TypeParam> output = c.Forward({input, weigths});

  ASSERT_EQ(output->shape().size(), 3);
  EXPECT_EQ(output->shape()[0], 1);
  EXPECT_EQ(output->shape()[1], 1);
  EXPECT_EQ(output->shape()[2], 1);
  EXPECT_EQ(output->At(0), typename TypeParam::Type(-20));
}

TYPED_TEST(ConvolutionTest, forward_1x3x3_1x1x3x3)
{
  std::shared_ptr<TypeParam> input = std::make_shared<TypeParam>(std::vector<uint64_t>({1, 3, 3}));
  std::shared_ptr<TypeParam> weigths = std::make_shared<TypeParam>(std::vector<uint64_t>({1, 1, 3, 3}));
  for (uint64_t i(0) ; i < 9 ; ++i)
    {
      input->At(i) = typename TypeParam::Type(i);
      weigths->At(i) = typename TypeParam::Type(i);
    }
  fetch::ml::ops::Convolution<TypeParam> c;
  std::shared_ptr<TypeParam> output = c.Forward({input, weigths});
  
  ASSERT_EQ(output->shape().size(), 3);
  EXPECT_EQ(output->shape()[0], 1);
  EXPECT_EQ(output->shape()[1], 1);
  EXPECT_EQ(output->shape()[2], 1);
  EXPECT_EQ(output->At(0), typename TypeParam::Type(204));
}

TYPED_TEST(ConvolutionTest, forward_3x3x3_1x3x3x3)
{
  std::shared_ptr<TypeParam> input = std::make_shared<TypeParam>(std::vector<uint64_t>({3, 3, 3}));
  std::shared_ptr<TypeParam> weigths = std::make_shared<TypeParam>(std::vector<uint64_t>({1, 3, 3, 3}));
  for (uint64_t i(0) ; i < 27 ; ++i)
    {
      input->At(i) = typename TypeParam::Type(i);
      weigths->At(i) = typename TypeParam::Type(i);
    }
  fetch::ml::ops::Convolution<TypeParam> c;
  std::shared_ptr<TypeParam> output = c.Forward({input, weigths});

  ASSERT_EQ(output->shape().size(), 3);
  EXPECT_EQ(output->shape()[0], 1);
  EXPECT_EQ(output->shape()[1], 1);
  EXPECT_EQ(output->shape()[2], 1);
  EXPECT_EQ(output->At(0), typename TypeParam::Type(6201));
}

TYPED_TEST(ConvolutionTest, forward_3x3x3_5x3x3x3)
{
  std::shared_ptr<TypeParam> input = std::make_shared<TypeParam>(std::vector<uint64_t>({3, 3, 3}));
  std::shared_ptr<TypeParam> weigths = std::make_shared<TypeParam>(std::vector<uint64_t>({5, 3, 3, 3}));
  fetch::ml::ops::Convolution<TypeParam> c;
  std::shared_ptr<TypeParam> output = c.Forward({input, weigths});

  ASSERT_EQ(output->shape().size(), 3);
  EXPECT_EQ(output->shape()[0], 5);
  EXPECT_EQ(output->shape()[1], 1);
  EXPECT_EQ(output->shape()[2], 1);
}

TYPED_TEST(ConvolutionTest, forward_1x5x5_1x1x3x3)
{
  std::shared_ptr<TypeParam> input = std::make_shared<TypeParam>(std::vector<uint64_t>({1, 5, 5}));
  std::shared_ptr<TypeParam> weigths = std::make_shared<TypeParam>(std::vector<uint64_t>({1, 1, 3, 3}));
  fetch::ml::ops::Convolution<TypeParam> c;
  std::shared_ptr<TypeParam> output = c.Forward({input, weigths});

  ASSERT_EQ(output->shape().size(), 3);
  EXPECT_EQ(output->shape()[0], 1);
  EXPECT_EQ(output->shape()[1], 3);
  EXPECT_EQ(output->shape()[2], 3);
}
