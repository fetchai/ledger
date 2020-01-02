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

#include "math/distance/square.hpp"
#include "math/fundamental_operators.hpp"

#include <cassert>

namespace fetch {
namespace math {

/**
 * This implementation assumes that the trailing dimension is the data/batch dimension, and all
 * other dimensions must therefore be feature dimensions
 * @tparam ArrayType
 * @param A Testing data tensor
 * @param B Ground truth data tensor
 * @return
 */
template <typename ArrayType>
typename ArrayType::Type MeanSquareError(ArrayType const &A, ArrayType const &B)
{
  using DataType = typename ArrayType::Type;
  assert(A.shape() == B.shape());

  // compute the squared distance
  DataType ret = distance::SquareDistance(A, B);

  // divide by data size to get the Mean
  Divide(ret, static_cast<DataType>(A.size()), ret);

  return ret;
}

}  // namespace math
}  // namespace fetch
