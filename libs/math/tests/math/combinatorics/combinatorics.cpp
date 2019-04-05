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

#include "math/combinatorics.hpp"
#include <math/tensor.hpp>

using namespace fetch::math::combinatorics;

using DataType  = double;
using ArrayType = typename fetch::math::Tensor<DataType>;
using SizeType  = std::uint64_t;

template <typename T>
class CombinatoricsTest : public ::testing::Test
{
};

using MyTypes =
    ::testing::Types<fetch::math::Tensor<std::int32_t>, fetch::math::Tensor<std::int64_t>,
                     fetch::math::Tensor<std::uint32_t>, fetch::math::Tensor<std::uint64_t>,
                     fetch::math::Tensor<float>, fetch::math::Tensor<double>>;
TYPED_TEST_CASE(CombinatoricsTest, MyTypes);

// Factorial function - test edge case (0!)
TYPED_TEST(CombinatoricsTest, test_factorial_zero)
{
  SizeType input  = 0;
  SizeType output = factorial(input);
  ASSERT_EQ(output, 1);
}

// Factorial function - test standard input (12!)
TYPED_TEST(CombinatoricsTest, test_factorial_standard_input)
{
  SizeType input        = 12;
  SizeType output       = factorial(input);
  SizeType numpy_output = 479001600;
  ASSERT_EQ(output, numpy_output);
}

// calculateNumCombinations function - test standard input
TYPED_TEST(CombinatoricsTest, test_num_combinations_standard_input)
{
  SizeType n = 5;
  SizeType r = 2;

  SizeType fetch_output  = calculateNumCombinations(n, r);
  SizeType python_output = 10;

  ASSERT_EQ(fetch_output, python_output);

  fetch_output = calculateNumCombinations(SizeType(9), SizeType(4));

  ASSERT_EQ(fetch_output, 126);

  n                     = (1 << 24) + 1;
  SizeType matrixHeight = static_cast<SizeType>(calculateNumCombinations(n, SizeType(1)));
  EXPECT_EQ(matrixHeight, n);

  if (sizeof(n) > 7)
  {
    n            = (1llu << 63) - 1;
    matrixHeight = static_cast<SizeType>(calculateNumCombinations(n, SizeType(1)));
    EXPECT_EQ(matrixHeight, n);

    n            = (1llu << 30) - 1;
    matrixHeight = static_cast<SizeType>(calculateNumCombinations(n, SizeType(2)));
    EXPECT_EQ(matrixHeight, n * (n - 1) / 2);
  }
}

// Combinations function - edge case - n=r
TYPED_TEST(CombinatoricsTest, test_num_combinations_edge_case1)
{
  SizeType n = 5;
  SizeType r = 5;

  SizeType fetch_output  = calculateNumCombinations(n, r);
  SizeType python_output = 1;

  ASSERT_EQ(fetch_output, python_output);
}

// Combinations function - edge case - n=r=1
TYPED_TEST(CombinatoricsTest, test_num_combinations_edge_case2)
{
  SizeType n = 1;
  SizeType r = 1;

  SizeType fetch_output  = calculateNumCombinations(n, r);
  SizeType python_output = 1;

  ASSERT_EQ(fetch_output, python_output);
}

// Combinations function - edge case - r=0
TYPED_TEST(CombinatoricsTest, test_num_combinations_edge_case3)
{
  SizeType n = 12;
  SizeType r = 0;

  SizeType fetch_output  = calculateNumCombinations(n, r);
  SizeType python_output = 1;

  ASSERT_EQ(fetch_output, python_output);
}

// Combinations function - test standard input
TYPED_TEST(CombinatoricsTest, test_combinations_standard_input)
{
  SizeType n = 5;
  SizeType r = 2;

  ArrayType python_output({2, 10});
  ArrayType fetch_output({2, 10});

  // Row 1
  python_output.Set({0, 0}, 4);
  python_output.Set({1, 0}, 5);

  // Row 2
  python_output.Set({0, 1}, 3);
  python_output.Set({1, 1}, 5);

  // Row 3
  python_output.Set({0, 2}, 3);
  python_output.Set({1, 2}, 4);

  // Row 4
  python_output.Set({0, 3}, 2);
  python_output.Set({1, 3}, 5);

  // Row 5
  python_output.Set({0, 4}, 2);
  python_output.Set({1, 4}, 4);

  // Row 6
  python_output.Set({0, 5}, 2);
  python_output.Set({1, 5}, 3);

  // Row 7
  python_output.Set({0, 6}, 1);
  python_output.Set({1, 6}, 5);

  // Row 8
  python_output.Set({0, 7}, 1);
  python_output.Set({1, 7}, 4);

  // Row 9
  python_output.Set({0, 8}, 1);
  python_output.Set({1, 8}, 3);

  // Row 10
  python_output.Set({0, 9}, 1);
  python_output.Set({1, 9}, 2);

  fetch_output = combinations<ArrayType>(n, r);

  ASSERT_TRUE(fetch_output.AllClose(python_output));
}

// Combinations function - edge case - n=r
TYPED_TEST(CombinatoricsTest, test_combinations_edge_case1)
{
  SizeType n = 5;
  SizeType r = 5;

  ArrayType python_output({5, 1});
  ArrayType fetch_output({5, 1});

  // Row 1
  python_output.Set({0, 0}, 1);
  python_output.Set({1, 0}, 2);
  python_output.Set({2, 0}, 3);
  python_output.Set({3, 0}, 4);
  python_output.Set({4, 0}, 5);

  fetch_output = combinations<ArrayType>(n, r);

  ASSERT_TRUE(fetch_output.AllClose(python_output));
}

// Combinations function - edge case - n=r=1
TYPED_TEST(CombinatoricsTest, test_combinations_edge_case2)
{
  SizeType n = 1;
  SizeType r = 1;

  ArrayType python_output({1, 1});
  ArrayType fetch_output({1, 1});

  // Row 1
  python_output.Set({0, 0}, 1);

  fetch_output = combinations<ArrayType>(n, r);
  ASSERT_TRUE(fetch_output.AllClose(python_output));
}

// Combinations function - edge case - r=0
TYPED_TEST(CombinatoricsTest, test_combinations_edge_case3)
{
  SizeType n = 12;
  SizeType r = 0;

  ArrayType python_output{};
  ArrayType fetch_output{};

  fetch_output = combinations<ArrayType>(n, r);
  ASSERT_TRUE(fetch_output.AllClose(python_output));
}
