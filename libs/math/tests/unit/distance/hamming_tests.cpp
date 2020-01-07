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

#include "core/random/lcg.hpp"
#include "gtest/gtest.h"
#include "math/distance/hamming.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class HammingTest : public ::testing::Test
{
};

TYPED_TEST_CASE(HammingTest, TensorFloatingTypes);

TEST(HammingTest, simple_test)
{
  Tensor<double> A = Tensor<double>(4);
  A.Set(SizeType{0}, 1);
  A.Set(SizeType{1}, 2);
  A.Set(SizeType{2}, 3);
  A.Set(SizeType{3}, 4);
  EXPECT_EQ(distance::Hamming(A, A), 0);

  Tensor<double> B = Tensor<double>(4);
  B.Set(SizeType{0}, 1);
  B.Set(SizeType{1}, 2);
  B.Set(SizeType{2}, 3);
  B.Set(SizeType{3}, 2);

  EXPECT_EQ(distance::Hamming(A, B), 1);

  Tensor<double> C = Tensor<double>(3);
  C.Set(SizeType{0}, 1);
  C.Set(SizeType{1}, 2);
  C.Set(SizeType{2}, 3);

  Tensor<double> D = Tensor<double>(3);
  D.Set(SizeType{0}, 1);
  D.Set(SizeType{1}, 2);
  D.Set(SizeType{2}, 9);

  EXPECT_EQ(distance::Hamming(C, D), 1);
}

}  // namespace test
}  // namespace math
}  // namespace fetch
