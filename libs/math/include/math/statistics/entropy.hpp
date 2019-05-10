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

#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include <math/standard_functions/log.hpp>

#include <cmath>

namespace fetch {
namespace math {
namespace statistics {

/**
 * Calculates the Shannon entropy of probability values array a
 * i.e. -sum(a*log2(a))
 * @param a input array of probabilities
 * @param ret return value
 */
template <typename ArrayType>
void Entropy(ArrayType const &a, typename ArrayType::Type &ret)
{
  using DataType = typename ArrayType::Type;
  ret            = Multiply(DataType(-1), Sum(Multiply(a, fetch::math::Log2(a))));
}

template <typename ArrayType>
typename ArrayType::Type Entropy(ArrayType const &a)
{
  typename ArrayType::Type ret;
  Entropy(a, ret);
  return ret;
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch
