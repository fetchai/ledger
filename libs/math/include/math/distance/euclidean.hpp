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
#include "math/distance/square.hpp"
#include "math/matrix_operations.hpp"
#include "math/standard_functions/pow.hpp"
#include "math/standard_functions/sqrt.hpp"

#include <cmath>
#include <vector>

namespace fetch {
namespace math {
namespace distance {

template <typename ArrayType>
typename ArrayType::Type Euclidean(ArrayType const &A, ArrayType const &B)
{
  return Sqrt(SquareDistance(A, B));
}

template <typename ArrayType>
typename ArrayType::Type NegativeSquareEuclidean(ArrayType const &A, ArrayType const &B)
{
  using DataType = typename ArrayType::Type;
  return Multiply(DataType(-1), SquareDistance(A, B));
}

/**
 * calculate the euclidean distance between two points in N-dimensions
 * If the array has shape Kx1 or 1xK, the array is treated as 1 data point with K dimensions
 * If the array has shape MxN, axis determines whether M represents dimensions or data points
 * @tparam ArrayType
 * @param A
 * @param B
 * @param axis the axis across which to calculate euclidean distances (i.e. the dimension axis)
 * @return
 */
template <typename ArrayType>
ArrayType EuclideanMatrix(ArrayType const &A, ArrayType const &B,
                          typename ArrayType::SizeType const &axis = 1)
{
  assert(A.shape() == B.shape());
  assert(A.shape().size() == 2);
  assert(axis == 0 || axis == 1);

  ArrayType                                 temp(A.shape());
  std::vector<typename ArrayType::SizeType> retSize;
  if ((A.shape()[0] == 1) || (A.shape()[1] == 1))  // case where one dimension = size 1
  {
    retSize = A.shape();
  }
  else  // case where two dimensions, neither size 1, euclid across axis
  {
    if (axis == 0)
    {
      retSize = std::vector<typename ArrayType::SizeType>({1, A.shape()[1]});
    }
    else
    {
      retSize = std::vector<typename ArrayType::SizeType>({A.shape()[0], 1});
    }
  }
  ArrayType ret(retSize);
  Subtract(A, B, temp);
  Square(temp, temp);
  ret = ReduceSum(temp, axis);
  Sqrt(ret, ret);

  return ret;
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
