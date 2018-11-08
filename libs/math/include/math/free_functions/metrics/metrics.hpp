#pragma once
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

#include <iostream>
#include <cassert>

namespace fetch {
namespace math {
namespace metrics {

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
ArrayType EuclideanDistance(ArrayType const &A, ArrayType const &B, std::size_t const &axis = 1)
{
  assert(A.shape() == B.shape());
  assert(A.shape().size() == 2);
  assert(axis == 0 || axis == 1);

  ArrayType temp{A.shape()};
  ArrayType ret{};
  if ((A.shape()[0] == 1) || (A.shape()[1] == 1))  // case where one dimension = size 1
  {
    ret.Reshape(A.shape());
  }
  else  // case where two dimensions, neither size 1, euclid across axis
  {
    if (axis == 0)
    {
      ret.Reshape({1, A.shape()[1]});
    }
    else
    {
      ret.Reshape({A.shape()[0], 1});
    }
  }
  Subtract(A, B, temp);
  Square(temp);
  ret = ReduceSum(temp, axis);
  Sqrt(ret);

  return ret;
}

}  // namespace metrics
}  // namespace math
}  // namespace fetch