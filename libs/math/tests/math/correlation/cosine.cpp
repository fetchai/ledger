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
#include <iostream>

#include "math/correlation/cosine.hpp"
#include "math/tensor.hpp"

using namespace fetch::math;
using namespace fetch::math::correlation;

TEST(distance_tests, cosine_distance)
{
  using SizeType = typename Tensor<double>::SizeType;

  Tensor<double> A = Tensor<double>({1, 4});
  A.Set(SizeType{0}, SizeType{0}, 1);
  A.Set(SizeType{0}, SizeType{1}, 2);
  A.Set(SizeType{0}, SizeType{2}, 3);
  A.Set(SizeType{0}, SizeType{3}, 4);

  Tensor<double> B = Tensor<double>({1, 4});
  B.Set(SizeType{0}, SizeType{0}, -1);
  B.Set(SizeType{0}, SizeType{1}, -2);
  B.Set(SizeType{0}, SizeType{2}, -3);
  B.Set(SizeType{0}, SizeType{3}, -4);

  ASSERT_EQ(Cosine(A, A), 1);
  ASSERT_EQ(Cosine(A, B), -1);

  Tensor<double> C = Tensor<double>({1, 4});
  C.Set(SizeType{0}, SizeType{0}, 1);
  C.Set(SizeType{0}, SizeType{1}, 2);
  C.Set(SizeType{0}, SizeType{2}, 3);
  C.Set(SizeType{0}, SizeType{3}, 2);

  EXPECT_NEAR(Cosine(A, C), double(0.94672926240625754), 1e-7);
}
