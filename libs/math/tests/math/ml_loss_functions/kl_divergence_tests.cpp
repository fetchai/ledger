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

#include "math/free_functions/ml/loss_functions/kl_divergence.hpp"
#include "math/tensor.hpp"

using namespace fetch::math::free_functions;
using namespace fetch::math;

template <typename T>
class KlDivergenceTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(KlDivergenceTest, MyTypes);

TYPED_TEST(KlDivergenceTest, same_tensors_divergence_test)
{
  Tensor<double> A = Tensor<double>({4, 4});

  A.Set({0, 0}, 0.1);
  A.Set({0, 1}, 0.2);
  A.Set({0, 2}, 0.3);
  A.Set({0, 3}, 0.4);

  A.Set({1, 0}, -0.1);
  A.Set({1, 1}, -0.2);
  A.Set({1, 2}, -0.3);
  A.Set({1, 3}, -0.4);

  A.Set({2, 0}, -1.1);
  A.Set({2, 1}, -1.2);
  A.Set({2, 2}, -1.3);
  A.Set({2, 3}, -1.4);

  A.Set({3, 0}, 1.1);
  A.Set({3, 1}, 1.2);
  A.Set({3, 2}, 1.3);
  A.Set({3, 3}, 1.4);

  Tensor<double> B = Tensor<double>({4, 4});

  B.Set({0, 0}, 0.1);
  B.Set({0, 1}, 0.2);
  B.Set({0, 2}, 0.3);
  B.Set({0, 3}, 0.4);

  B.Set({1, 0}, -0.1);
  B.Set({1, 1}, -0.2);
  B.Set({1, 2}, -0.3);
  B.Set({1, 3}, -0.4);

  B.Set({2, 0}, -1.1);
  B.Set({2, 1}, -1.2);
  B.Set({2, 2}, -1.3);
  B.Set({2, 3}, -1.4);

  B.Set({3, 0}, 1.1);
  B.Set({3, 1}, 1.2);
  B.Set({3, 2}, 1.3);
  B.Set({3, 3}, 1.4);

  EXPECT_NEAR(KlDivergence(A, B), 0.0, 1e-7);
  EXPECT_NEAR(KlDivergence(B, A), 0.0, 1e-7);
}

TYPED_TEST(KlDivergenceTest, other_divergence_test)
{
  Tensor<double> A = Tensor<double>({4, 4});

  A.Set({0, 0}, 0.1);
  A.Set({0, 1}, 0.2);
  A.Set({0, 2}, 0.3);
  A.Set({0, 3}, 0.4);

  A.Set({1, 0}, -0.1);
  A.Set({1, 1}, -0.2);
  A.Set({1, 2}, -0.3);
  A.Set({1, 3}, -0.4);

  A.Set({2, 0}, -1.1);
  A.Set({2, 1}, -1.2);
  A.Set({2, 2}, -1.3);
  A.Set({2, 3}, -1.4);

  A.Set({3, 0}, 1.1);
  A.Set({3, 1}, 1.2);
  A.Set({3, 2}, 1.3);
  A.Set({3, 3}, 1.4);

  Tensor<double> B = Tensor<double>({4, 2});

  B.Set({0, 0}, 1.1);
  B.Set({0, 1}, 1.2);

  B.Set({1, 0}, -1.1);
  B.Set({1, 1}, -1.2);

  B.Set({2, 0}, -0.1);
  B.Set({2, 1}, -0.2);

  B.Set({3, 0}, 0.1);
  B.Set({3, 1}, 0.2);

  EXPECT_NEAR(KlDivergence(A, B), 3.8460455925219406, 1e-7);
  EXPECT_NEAR(KlDivergence(B, A), 6.8452644686571968, 1e-7);
}