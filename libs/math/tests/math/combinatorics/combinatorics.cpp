//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "math/free_functions/combinatorics/combinatorics.hpp"
#include <math/linalg/matrix.hpp>

using namespace fetch::math::combinatorics;

// Factorial function - test edge case (0!)
TEST(combinatorics, test_factorial_zero)
{
  std::size_t input  = 0;
  std::size_t output = factorial(input);
  ASSERT_TRUE(output == 1);
}

// Factorial function - test standard input (12!)
TEST(combinatorics, test_factorial_standard_input)
{
  std::size_t input        = 12;
  std::size_t output       = factorial(input);
  std::size_t numpy_output = 479001600;
  ASSERT_TRUE(output == numpy_output);
}

// calculateNumCombinations function - test standard input
TEST(combinatorics, test_num_combinations_standard_input)
{
  std::size_t n = 5;
  std::size_t r = 2;

  float fetch_output  = calculateNumCombinations(n, r);
  float python_output = 10;

  ASSERT_TRUE(fetch_output == python_output);
}

// Combinations function - edge case - n=r
TEST(combinatorics, test_num_combinations_edge_case1)
{
  std::size_t n = 5;
  std::size_t r = 5;

  float fetch_output  = calculateNumCombinations(n, r);
  float python_output = 1;

  ASSERT_TRUE(fetch_output == python_output);
}

// Combinations function - edge case - n=r=1
TEST(combinatorics, test_num_combinations_edge_case2)
{
  std::size_t n = 1;
  std::size_t r = 1;

  float fetch_output  = calculateNumCombinations(n, r);
  float python_output = 1;

  ASSERT_TRUE(fetch_output == python_output);
}

// Combinations function - edge case - r=0
TEST(combinatorics, test_num_combinations_edge_case3)
{
  std::size_t n = 12;
  std::size_t r = 0;

  float fetch_output  = calculateNumCombinations(n, r);
  float python_output = 1;

  ASSERT_TRUE(fetch_output == python_output);
}

// Combinations function - test standard input
TEST(combinatorics, test_combinations_standard_input)
{
  std::size_t n = 5;
  std::size_t r = 2;

  fetch::math::linalg::Matrix<double> python_output{10, 2};
  fetch::math::linalg::Matrix<double> fetch_output{10, 2};

  // Row 1
  python_output.Set(0, 0, 4);
  python_output.Set(0, 1, 5);

  // Row 2
  python_output.Set(1, 0, 3);
  python_output.Set(1, 1, 5);

  // Row 3
  python_output.Set(2, 0, 3);
  python_output.Set(2, 1, 4);

  // Row 4
  python_output.Set(3, 0, 2);
  python_output.Set(3, 1, 5);

  // Row 5
  python_output.Set(4, 0, 2);
  python_output.Set(4, 1, 4);

  // Row 6
  python_output.Set(5, 0, 2);
  python_output.Set(5, 1, 3);

  // Row 7
  python_output.Set(6, 0, 1);
  python_output.Set(6, 1, 5);

  // Row 8
  python_output.Set(7, 0, 1);
  python_output.Set(7, 1, 4);

  // Row 9
  python_output.Set(8, 0, 1);
  python_output.Set(8, 1, 3);

  // Row 10
  python_output.Set(9, 0, 1);
  python_output.Set(9, 1, 2);

  fetch_output = combinations(n, r);
  ASSERT_TRUE(fetch_output == python_output);
}

// Combinations function - edge case - n=r
TEST(combinatorics, test_combinations_edge_case1)
{
  std::size_t n = 5;
  std::size_t r = 5;

  fetch::math::linalg::Matrix<double> python_output{1, 5};
  fetch::math::linalg::Matrix<double> fetch_output{1, 5};

  // Row 1
  python_output.Set(0, 0, 1);
  python_output.Set(0, 1, 2);
  python_output.Set(0, 2, 3);
  python_output.Set(0, 3, 4);
  python_output.Set(0, 4, 5);

  fetch_output = combinations(n, r);
  ASSERT_TRUE(fetch_output == python_output);
}

// Combinations function - edge case - n=r=1
TEST(combinatorics, test_combinations_edge_case2)
{
  std::size_t n = 1;
  std::size_t r = 1;

  fetch::math::linalg::Matrix<double> python_output{1, 1};
  fetch::math::linalg::Matrix<double> fetch_output{1, 1};

  // Row 1
  python_output.Set(0, 0, 1);

  fetch_output = combinations(n, r);
  ASSERT_TRUE(fetch_output == python_output);
}

// Combinations function - edge case - r=0
TEST(combinatorics, test_combinations_edge_case3)
{
  std::size_t n = 12;
  std::size_t r = 0;

  fetch::math::linalg::Matrix<double> python_output{};
  fetch::math::linalg::Matrix<double> fetch_output{};

  fetch_output = combinations(n, r);
  ASSERT_TRUE(fetch_output == python_output);
}