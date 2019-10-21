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

#include "math/correlation/pearson.hpp"
#include "math/tensor.hpp"

#include "gtest/gtest.h"

using namespace fetch::math;
using namespace fetch::math::correlation;

template <typename T>
class PearsonCorrelationTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(PearsonCorrelationTest, MyTypes);

TYPED_TEST(PearsonCorrelationTest, simple_test)
{
  Tensor<double> A{3};
  A.Fill(0.0);
  A.At(0) = 1.0;
  EXPECT_FLOAT_EQ(float(1), float(Pearson(A, A)));
}

TEST(correlation_gtest, pearson_correlation_test)
{
  Tensor<double> A{3};
  Tensor<double> B{3};
  A.At(0) = 1.0;
  A.At(1) = 2.0;
  A.At(2) = 3.0;
  B.At(0) = 3.0;
  B.At(1) = 2.0;
  B.At(2) = 1.0;
  EXPECT_FLOAT_EQ(float(-1), float(Pearson(A, B)));
}
