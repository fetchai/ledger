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

namespace fetch {
namespace math {

/**
 * One hot function based on tf.one_hot
 * @tparam ArrayType
 * @param ret output tensor
 * @param indices input tensorSizeType
 * @param depth number of classes
 * @param on_value TRUE value
 * @param off_value FALSE value
 * @param axis
 */
template <typename ArrayType>
void OneHot(ArrayType &ret, ArrayType const &indices, typename ArrayType::SizeType depth,
            SizeType axis = 0, typename ArrayType::Type on_value = typename ArrayType::Type{1.0},
            typename ArrayType::Type off_value = typename ArrayType::Type{0.0})
{
  assert((indices.shape().size() + 1) == ret.shape().size());
  assert(axis <= indices.size());
  assert(depth == ret.shape().at(axis));

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
    // Check if class is no out of range
    assert(static_cast<SizeType>(*ind_it) < depth);

    // Iterate over FALSE values before one TRUE value
    for (SizeType i{0}; i < (static_cast<SizeType>(*ind_it)); i++)
    {
      *ret_it = off_value;
      ++ret_it;
    }

    // Set one TRUE value
    *ret_it = on_value;
    ++ret_it;

    // Iterate over rest of FALSE values after one TRUE value
    for (SizeType i{1}; i < depth - (static_cast<SizeType>(*ind_it)); i++)
    {
      *ret_it = off_value;
      ++ret_it;
    }

    ++ind_it;
  }
}

/**
 * One hot function based on tf.one_hot
 * @tparam ArrayType
 * @param indices input tensor
 * @param depth number of classes
 * @param on_value TRUE value
 * @param off_value FALSE value
 * @param axis
 * @return
 */
template <typename ArrayType>
ArrayType OneHot(ArrayType const &indices, typename ArrayType::SizeType depth, SizeType axis = 0,
                 typename ArrayType::Type on_value  = typename ArrayType::Type{1},
                 typename ArrayType::Type off_value = typename ArrayType::Type{0})
{
  assert(axis <= indices.shape().size());

  std::vector<SizeType> ret_shape = indices.shape();

  if (axis == indices.shape().size())
  {
    ret_shape.emplace_back(depth);
  }
  else
  {
    ret_shape.insert(ret_shape.begin() + static_cast<PtrDiffType>(axis), depth);
  }

  ArrayType ret(ret_shape);

  OneHot(ret, indices, depth, axis, on_value, off_value);
  return ret;
}

}  // namespace math
}  // namespace fetch
