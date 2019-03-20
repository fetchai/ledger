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

#include "core/assert.hpp"
#include "math/distance/euclidean.hpp"
#include "math/free_functions/fundamental_operators.hpp"
#include "math/free_functions/matrix_operations/matrix_operations.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename ArrayType>
void ConditionalProbabilitiesDistance(ArrayType const &a, size_t i, size_t j,
                                      typename ArrayType::Type const sigma,
                                      typename ArrayType::Type &     r)
{
  assert(i < a.shape()[0] && j < a.shape()[0]);

  // Calculate numerator
  typename ArrayType::Type tmp_numerator = SquareDistance(a.Slice(i), a.Slice(j));

  // Divide by 2 sigma squared
  tmp_numerator = -Divide(tmp_numerator, 2.0 * sigma * sigma);

  Exp(tmp_numerator, tmp_numerator);

  // Calculate denominator
  typename ArrayType::Type tmp_denominator = 0.0;

  for (size_t k = 0; k < a.shape()[0]; k++)
  {
    if (k == i)
      continue;
    typename ArrayType::Type tmp_val = SquareDistance(a.Slice(i), a.Slice(k));
    tmp_val                          = -Divide(tmp_val, 2.0 * sigma * sigma);
    Exp(tmp_val, tmp_val);

    tmp_denominator += tmp_val;
  }

  r = tmp_numerator / tmp_denominator;
}

template <typename ArrayType>
typename ArrayType::Type ConditionalProbabilitiesDistance(ArrayType const &a, int i, int j,
                                                          typename ArrayType::Type const sigma)
{
  typename ArrayType::Type ret;
  ConditionalProbabilitiesDistance(a, i, j, sigma, ret);
  return ret;
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
