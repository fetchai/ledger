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

/**
 * Calculates the conditional probability of point j given point i in N-dimensional feature space
 * i.e. e^(- x_i^2 / 2sigma^2) / Sum(e^(- x_j^2 / 2sigma^2))
 * @param a input tensor of dimensions n_data x n_features
 * @param i index of the data point on which distribution is centred
 * @param j index of data point for which to estimate probability
 * @param sigma controls the distribution kurtosis/width
 * @param ret return value
 */
template <typename ArrayType>
void ConditionalProbabilitiesDistance(ArrayType const &a, std::size_t i, std::size_t j,
                                      typename ArrayType::Type const sigma,
                                      typename ArrayType::Type &     ret)
{
  assert(i < a.shape().at(0) && j < a.shape().at(0));

  // Calculate numerator
  typename ArrayType::Type tmp_numerator = SquareDistance(a.Slice(i), a.Slice(j));

  // Divide by 2 sigma squared and make negative
  tmp_numerator =
      -Divide(tmp_numerator, Multiply(typename ArrayType::Type(2.0), Multiply(sigma, sigma)));

  Exp(tmp_numerator, tmp_numerator);

  // Calculate denominator
  typename ArrayType::Type tmp_denominator(0.0);

  for (size_t k = 0; k < a.shape().at(0); k++)
  {
    if (k == i)
      continue;
    typename ArrayType::Type tmp_val = SquareDistance(a.Slice(i), a.Slice(k));
    tmp_val = -Divide(tmp_val, Multiply(typename ArrayType::Type(2.0), Multiply(sigma, sigma)));
    Exp(tmp_val, tmp_val);

    tmp_denominator += tmp_val;
  }

  ret = tmp_numerator / tmp_denominator;
}

template <typename ArrayType>
typename ArrayType::Type ConditionalProbabilitiesDistance(ArrayType const &a, std::size_t i,
                                                          std::size_t                    j,
                                                          typename ArrayType::Type const sigma)
{
  typename ArrayType::Type ret;
  ConditionalProbabilitiesDistance(a, i, j, sigma, ret);
  return ret;
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
