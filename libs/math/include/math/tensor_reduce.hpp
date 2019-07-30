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

#include "math/base_types.hpp"
#include "math/tensor_declaration.hpp"
#include "math/tensor_slice_iterator.hpp"

#include <cassert>

namespace fetch {
namespace math {

/**
 * Applies given function along given axis on N-1 sized array
 * @tparam F Function type
 * @tparam T
 * @tparam C
 * @param axis Axis along which function will be applied
 * @param function Function that will be applied along specified axis
 * @param array Constant input tensor
 * @param ret Output tensor
 */
template <typename F, typename T, typename C>
inline void Reduce(SizeType axis, F function, const Tensor<T, C> &array, Tensor<T, C> &ret)
{
  assert(ret.shape().at(axis) == 1);
  for (SizeType i = 0; i < array.shape().size(); i++)
  {
    if (i != axis)
    {
      assert(array.shape().at(i) == ret.shape().at(i));
    }
  }

  // If axis is 0, no need to put axis to front
  if (axis == 0)
  {
    auto it_a = array.cbegin();
    auto it_b = ret.begin();

    while (it_a.is_valid())
    {
      // Move return array iterator every array.shape().at(axis) steps
      for (SizeType j{0}; j < array.shape().at(0); ++j)
      {
        // Apply function
        function(*it_a, *it_b);
        ++it_a;
      }
      ++it_b;
    }
  }
  else
  {
    // Create axis-permutable slice iterator
    auto a_it = array.Slice().cbegin();
    auto r_it = ret.Slice().begin();

    // Put selected axis to front
    a_it.PermuteAxes(0, axis);
    r_it.PermuteAxes(0, axis);

    while (a_it.is_valid())
    {
      // Move return array iterator every array.shape().at(axis) steps
      for (SizeType j{0}; j < array.shape().at(axis); ++j)
      {
        // Apply function
        function(*a_it, *r_it);
        ++a_it;
      }
      ++r_it;
    }
  }
}

}  // namespace math
}  // namespace fetch
