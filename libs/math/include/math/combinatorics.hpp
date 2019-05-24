#pragma once
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

#include <cassert>
#include <iostream>

#include "math/tensor.hpp"

namespace fetch {
namespace math {
namespace combinatorics {

/**
 * Calculates the factorial of an integer (n!)
 * @param n - Integer for which this function calculates the factorial
 * @return - Integer solution to n!
 */
std::size_t factorial(std::size_t n)
{
  // Trivial case
  if (n == 0)
  {
    return 1;
  }

  // General solution
  for (std::size_t i = n - 1; i > 1; i--)
  {
    n *= i;
  }

  return n;
}

/**
 * Recursively calculates "n Choose r" in a safe and efficient way, avoiding potential overflow
 * errors from factorial function
 * @param n - The total number of items to choose from
 * @param r - The size of each combination
 * @return - Number of combinations as float
 */
std::size_t calculateNumCombinations(std::size_t n, std::size_t r)
{
  assert(r <= n);

  // Base case
  switch (r)
  {
  case 0:
    return 1;
  case 1:
    return n;
  case 2:
    return n % 2 ? n * ((n - 1) >> 1) : (n >> 1) * (n - 1);
  }

  // Sometimes faster to calculate equivalent definition "n Choose n-r" than "n Choose r"
  if (r > n / 2)
  {
    return calculateNumCombinations(n, n - r);
  }

  // Recursive implementation
  double fraction = static_cast<double>(n) / static_cast<double>(r);
  while (--r > 1)
  {
    fraction *= static_cast<double>(--n) / static_cast<double>(r);
  }
  return static_cast<std::size_t>(fraction * static_cast<double>(--n));
}

/**
 * Calculates all size r combinations of items [1,...,n]
 * @param n - The total number of items to choose from, where items will be selected from [1,...,n]
 * @param r - The number of items to select
 * @return - Matrix of size (num possible combinations, r), where each row contains a unique
 * combination of r items
 */
template <typename ArrayType>
ArrayType combinations(std::size_t n, std::size_t r)
{
  assert(r <= n);
  if (r == 0)
  {
    ArrayType output_array({});
    return output_array;
  }

  std::size_t n_combinations = calculateNumCombinations(n, r);
  std::size_t current_dim    = 0;
  std::size_t current_row    = 0;

  std::vector<bool> v(n);
  std::fill(v.end() - static_cast<int>(r), v.end(), true);

  ArrayType output_array({r, n_combinations});
  do
  {
    for (std::size_t i = 0; i < n; ++i)
    {
      if (v[i])
      {
        std::size_t dim = (i + 1);
        output_array.Set(current_dim, current_row, static_cast<float>(dim));
        if (current_dim == r - 1)
        {
          ++current_row;
          current_dim = 0;
          break;
        }
        ++current_dim;
      }
    }
  } while (std::next_permutation(v.begin(), v.end()));
  return output_array;
}

}  // namespace combinatorics
}  // namespace math
}  // namespace fetch
