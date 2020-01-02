#pragma once
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

#include "core/assert.hpp"
#include "math/standard_functions/pow.hpp"
#include "math/statistics/entropy.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace statistics {

/**
 * Calculates the Perplexity of Shannon entropy for point i in N-dimensional feature space
 * i.e. 2^Entropy(i)
 * @param a input tensor of dimensions n_data x n_features
 * @param i index of the data point on which distribution is centred
 * @param ret return value
 */
template <typename ArrayType>
void Perplexity(ArrayType const &a, typename ArrayType::Type &ret)
{
  using DataType = typename ArrayType::Type;
  Pow(DataType(2), Entropy(a), ret);
}

template <typename ArrayType>
typename ArrayType::Type Perplexity(ArrayType const &a)
{
  typename ArrayType::Type ret;
  Perplexity(a, ret);
  return ret;
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch
