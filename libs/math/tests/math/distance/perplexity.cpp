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

#include <gtest/gtest.h>

#include "math/statistics/perplexity.hpp"
#include "math/tensor.hpp"

using namespace fetch::math::statistics;
using namespace fetch::math;

template <typename T>
class PerplexityTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(PerplexityTest, MyTypes);

TYPED_TEST(PerplexityTest, entropy)
{
  using DataType  = typename TypeParam::Type;
  using SizeType  = typename TypeParam::SizeType;
  using ArrayType = TypeParam;

  ArrayType A(4);

  A.Set(SizeType{0}, DataType(0.1));
  A.Set(SizeType{1}, DataType(0.2));
  A.Set(SizeType{2}, DataType(0.3));
  A.Set(SizeType{3}, DataType(0.4));

  EXPECT_NEAR(double(Perplexity(A)), 3.59611546662432, 1e-3);
}