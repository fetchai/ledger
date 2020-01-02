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

#include "math/base_types.hpp"
#include "math/tensor/tensor_declaration.hpp"
#include "math/tensor/tensor_slice_iterator.hpp"

#include <cassert>

namespace fetch {
namespace math {

/**
 * Applies given function along given axis resulting in a N-1 sized array
 * @tparam F Function type
 * @tparam T
 * @tparam C
 * @param axis Axis along which function will be applied
 * @param function Function that will be applied along specified axis
 * @param array Constant input tensor
 * @param ret Output tensor
 */
template <typename F, typename T, typename C>
void Reduce(SizeType axis, F function, const Tensor<T, C> &array, Tensor<T, C> &ret)
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

/**
 * Applies given function along given axes on N-M sized array, where M is number of given axes
 * @tparam F Function type
 * @tparam T
 * @tparam C
 * @param axis Axis along which function will be applied
 * @param function Function that will be applied along specified axis
 * @param array Constant input tensor
 * @param ret Output tensor
 */
template <typename F, typename T, typename C>
void Reduce(std::vector<SizeType> axes, F function, const Tensor<T, C> &array, Tensor<T, C> &ret)
{
  for (SizeType i{0}; i < axes.size(); i++)
  {
    assert(ret.shape().at(axes.at(i)) == 1);

    // Axes must be sorted
    if (i != (axes.size() - 1))
    {
      assert(axes.at(i) < axes.at(i + 1));
    }
  }

  // If axes are not given like
  bool permuting_needed = false;
  for (SizeType i{0}; i < axes.size(); i++)
  {
    if (axes.at(i) != i)
    {
      permuting_needed = true;
      break;
    }
  }

  if (permuting_needed)
  {
    // Create axis-permutable slice iterator
    auto a_it = array.Slice().cbegin();
    auto r_it = ret.Slice().begin();

    SizeType n = 1;

    for (SizeType i{0}; i < axes.size(); i++)
    {
      // Calculate number of steps between iterations
      n *= array.shape().at(axes.at(i));

      // Put selected axes to front
      a_it.PermuteAxes(i, axes.at(i));
      r_it.PermuteAxes(i, axes.at(i));
    }

    while (a_it.is_valid())
    {
      // Move return array iterator every Nth steps
      for (SizeType j{0}; j < n; ++j)
      {
        // Apply function
        function(*a_it, *r_it);
        ++a_it;
      }
      ++r_it;
    }
  }
  else
  {
    // Create iterators
    auto a_it = array.cbegin();
    auto r_it = ret.begin();

    // Calculate number of steps between iterations
    SizeType n = 1;
    for (SizeType i{0}; i < axes.size(); i++)
    {
      n *= array.shape().at(axes.at(i));
    }

    while (a_it.is_valid())
    {
      // Move return array iterator every Nth steps
      for (SizeType j{0}; j < n; ++j)
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
