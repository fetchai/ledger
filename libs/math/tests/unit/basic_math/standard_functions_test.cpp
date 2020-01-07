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

#include "gtest/gtest.h"
#include "math/standard_functions/clamp.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class StandardFunctionTests : public ::testing::Test
{
};

TYPED_TEST_CASE(StandardFunctionTests, TensorFloatingTypes);

template <typename TensorType>
void RandomAssign(TensorType &tensor)
{
  using Type = typename TensorType::Type;

  auto it = tensor.begin();
  while (it.is_valid())
  {
    *it = random::Random::generator.AsType<Type>();
    ++it;
  }
}

TYPED_TEST(StandardFunctionTests, abs_test)
{
  using ArrayType = TypeParam;
  using Type      = typename ArrayType::Type;

  // randomly assign data to tensor
  ArrayType tensor = ArrayType({100});
  RandomAssign(tensor);

  // manually calculate the abs as ground truth comparison
  ArrayType gt    = tensor.Copy();
  auto      gt_it = gt.begin();
  while (gt_it.is_valid())
  {
    if (*gt_it < Type{0})
    {
      *gt_it = ((*gt_it) * Type{-1});
    }
    ++gt_it;
  }

  EXPECT_EQ(tensor, gt);
}

TYPED_TEST(StandardFunctionTests, clamp_array_1D_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType A = ArrayType({6});

  A(0) = DataType{-10};
  A(1) = DataType{0};
  A(2) = DataType{1};
  A(3) = DataType{2};
  A(4) = DataType{3};
  A(5) = DataType{10};

  // Expected results
  ArrayType A_clamp_expected = ArrayType({6});
  A_clamp_expected(0)        = DataType{2};
  A_clamp_expected(1)        = DataType{2};
  A_clamp_expected(2)        = DataType{2};
  A_clamp_expected(3)        = DataType{2};
  A_clamp_expected(4)        = DataType{3};
  A_clamp_expected(5)        = DataType{3};

  // Compare results with expected results
  Clamp(DataType{2}, DataType{3}, A);

  EXPECT_EQ(A, A_clamp_expected);
}

TYPED_TEST(StandardFunctionTests, clamp_array_2D_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType A = ArrayType({2, 3});

  A(0, 0) = DataType{-10};
  A(0, 1) = DataType{0};
  A(0, 2) = DataType{1};
  A(1, 0) = DataType{2};
  A(1, 1) = DataType{3};
  A(1, 2) = DataType{10};

  // Expected results
  ArrayType A_clamp_expected = ArrayType({2, 3});
  A_clamp_expected(0, 0)     = DataType{2};
  A_clamp_expected(0, 1)     = DataType{2};
  A_clamp_expected(0, 2)     = DataType{2};
  A_clamp_expected(1, 0)     = DataType{2};
  A_clamp_expected(1, 1)     = DataType{3};
  A_clamp_expected(1, 2)     = DataType{3};

  // Compare results with expected results
  Clamp(DataType{2}, DataType{3}, A);

  EXPECT_EQ(A, A_clamp_expected);
}

}  // namespace test
}  // namespace math
}  // namespace fetch
