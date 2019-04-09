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
#include <iomanip>
#include <iostream>

#include "math/ml/loss_functions/kl_divergence.hpp"
#include "math/tensor.hpp"

using namespace fetch::math;

template <typename T>
class KlDivergenceTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(KlDivergenceTest, MyTypes);

TYPED_TEST(KlDivergenceTest, same_tensors_divergence_test)
{
  TypeParam A({4, 4});

  A.Set({0, 0}, typename TypeParam::Type(0.1));
  A.Set({0, 1}, typename TypeParam::Type(0.2));
  A.Set({0, 2}, typename TypeParam::Type(0.3));
  A.Set({0, 3}, typename TypeParam::Type(0.4));

  A.Set({1, 0}, typename TypeParam::Type(-0.1));
  A.Set({1, 1}, typename TypeParam::Type(-0.2));
  A.Set({1, 2}, typename TypeParam::Type(-0.3));
  A.Set({1, 3}, typename TypeParam::Type(-0.4));

  A.Set({2, 0}, typename TypeParam::Type(-1.1));
  A.Set({2, 1}, typename TypeParam::Type(-1.2));
  A.Set({2, 2}, typename TypeParam::Type(-1.3));
  A.Set({2, 3}, typename TypeParam::Type(-1.4));

  A.Set({3, 0}, typename TypeParam::Type(1.1));
  A.Set({3, 1}, typename TypeParam::Type(1.2));
  A.Set({3, 2}, typename TypeParam::Type(1.3));
  A.Set({3, 3}, typename TypeParam::Type(1.4));

  TypeParam B = A.Copy();

  EXPECT_NEAR(double(KlDivergence(A, B)), 0.0, 1e-5);
  EXPECT_NEAR(double(KlDivergence(B, A)), 0.0, 1e-5);
}

TYPED_TEST(KlDivergenceTest, other_divergence_test)
{
  TypeParam A({4, 4});

  A.Set({0, 0}, typename TypeParam::Type(0.15));
  A.Set({0, 1}, typename TypeParam::Type(0.16));
  A.Set({0, 2}, typename TypeParam::Type(0.17));
  A.Set({0, 3}, typename TypeParam::Type(0.18));

  A.Set({1, 0}, typename TypeParam::Type(0.19));
  A.Set({1, 1}, typename TypeParam::Type(0.20));
  A.Set({1, 2}, typename TypeParam::Type(0.21));
  A.Set({1, 3}, typename TypeParam::Type(0.22));

  A.Set({2, 0}, typename TypeParam::Type(0.23));
  A.Set({2, 1}, typename TypeParam::Type(0.24));
  A.Set({2, 2}, typename TypeParam::Type(0.25));
  A.Set({2, 3}, typename TypeParam::Type(0.26));

  A.Set({3, 0}, typename TypeParam::Type(0.27));
  A.Set({3, 1}, typename TypeParam::Type(0.28));
  A.Set({3, 2}, typename TypeParam::Type(0.29));
  A.Set({3, 3}, typename TypeParam::Type(0.30));

  TypeParam B({4, 4});

  B.Set({0, 0}, typename TypeParam::Type(0.31));
  B.Set({0, 1}, typename TypeParam::Type(0.32));
  B.Set({0, 2}, typename TypeParam::Type(0.33));
  B.Set({0, 3}, typename TypeParam::Type(0.34));

  B.Set({1, 0}, typename TypeParam::Type(0.35));
  B.Set({1, 1}, typename TypeParam::Type(0.36));
  B.Set({1, 2}, typename TypeParam::Type(0.37));
  B.Set({1, 3}, typename TypeParam::Type(0.38));

  B.Set({2, 0}, typename TypeParam::Type(0.39));
  B.Set({2, 1}, typename TypeParam::Type(0.40));
  B.Set({2, 2}, typename TypeParam::Type(0.41));
  B.Set({2, 3}, typename TypeParam::Type(0.42));

  B.Set({3, 0}, typename TypeParam::Type(0.43));
  B.Set({3, 1}, typename TypeParam::Type(0.44));
  B.Set({3, 2}, typename TypeParam::Type(0.45));
  B.Set({3, 3}, typename TypeParam::Type(0.46));

  EXPECT_NEAR(double(KlDivergence(A, B)), -1.920114985949124, 1e-4);
  EXPECT_NEAR(double(KlDivergence(B, A)), 3.3324871063232422, 1e-4);
}