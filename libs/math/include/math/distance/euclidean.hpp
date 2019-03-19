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
#include "math/free_functions/exponentiation/exponentiation.hpp"
#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename ArrayType>
typename ArrayType::Type SquareDistance(ArrayType const &A, ArrayType const &B)
{
  assert(A.shape() == B.shape());
  ArrayType tmp_array(A.shape());

  fetch::math::Subtract(A, B, tmp_array);

  Square(tmp_array);

  return fetch::math::Sum(tmp_array);
}

template <typename ArrayType>
typename ArrayType::Type Euclidean(ArrayType const &A, ArrayType const &B)
{
  return Sqrt(SquareDistance(A, B));
}

template <typename ArrayType>
typename ArrayType::Type NegativeSquareEuclidean(ArrayType const &A, ArrayType const &B)
{
  return -SquareDistance(A, B);
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
