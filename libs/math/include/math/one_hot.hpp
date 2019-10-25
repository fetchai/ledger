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

#include <cassert>

namespace fetch {
namespace math {

/**
 * One hot function
 * @tparam ArrayType
 * @param ret
 * @param indices
 * @param depth
 * @param on_value
 * @param off_value
 * @param axis
 */
template <typename ArrayType>
void OneHot(ArrayType &ret, ArrayType const &indices, typename ArrayType::SizeType depth,
            typename ArrayType::Type on_value  = typename ArrayType::Type{1.0},
            typename ArrayType::Type off_value = typename ArrayType::Type{0.0}, SizeType axis = 0)
{
  auto ind_it = indices.begin();
  auto ret_it = ret.Slice().begin();

  if (axis != 0)
  {
    // Move the axis we want one-hot to the front
    // to make it iterable in the inner most loop.
    ret_it.MoveAxisToFront(axis);
  }

  while (ind_it.is_valid())
  {
    for (SizeType i{0}; i < (static_cast<SizeType>(*ind_it)); i++)
    {
      *ret_it = off_value;
      ++ret_it;
    }

    *ret_it = on_value;
    ++ret_it;

    for (SizeType i{1}; i < depth - (static_cast<SizeType>(*ind_it)); i++)
    {
      *ret_it = off_value;
      ++ret_it;
    }

    ++ind_it;
  }
}

/**
 * One hot function
 * @tparam ArrayType
 * @param indices
 * @param depth
 * @param on_value
 * @param off_value
 * @param axis
 * @return
 */
template <typename ArrayType>
ArrayType OneHot(ArrayType const &indices, typename ArrayType::SizeType depth,
                 typename ArrayType::Type on_value  = typename ArrayType::Type{1.0},
                 typename ArrayType::Type off_value = typename ArrayType::Type{0.0},
                 SizeType                 axis      = 0)
{
  using SizeType = typename ArrayType::SizeType;

  std::vector<SizeType> ret_shape = indices.shape();

  if (axis == indices.size())
  {
    ret_shape.emplace_back(depth);
  }
  else
  {
    ret_shape.insert(ret_shape.begin() + axis, depth);
  }

  ArrayType ret(ret_shape);

  OneHot(ret, indices, depth, on_value, off_value, axis);
  return ret;
}

}  // namespace math
}  // namespace fetch
