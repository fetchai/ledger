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

#include <cmath>

namespace fetch {
namespace math {

/**
 * Divide each value of an array by sum of all values
 * i.e. x / Sum(x)
 * @param a input array of values
 * @param ret return normalized value
 */
template <typename ArrayType>
void NormalizeArray(ArrayType const &a, ArrayType &ret)
{
  Divide(a, Sum(a), ret);
}

template <typename ArrayType>
ArrayType NormalizeArray(ArrayType const &a)
{
  ArrayType ret(a.shape());
  NormalizeArray(a, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
