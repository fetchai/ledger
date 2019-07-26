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

#include "math/comparison.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/meta/math_type_traits.hpp"
#include "math/standard_functions/exp.hpp"

#include <cassert>
#include <cstddef>
#include <stdexcept>

namespace fetch {
namespace math {

namespace details {

/*
 * Really naive implementation that relies only on ArrayType providing a At(std::size_t) method
 * TODO(private, 520) -- Clean up once we get unified ArrayType + operations
 */

template <typename ArrayType1, typename ArrayType2>
void Softmax1DImplementation(ArrayType1 const &array, ArrayType2 &ret)
{
  using Type = typename ArrayType1::Type;
  assert(ret.size() == array.size());

  // subtract max for numerical stability
  Type array_max = numeric_lowest<Type>();
  Max(array, array_max);

  auto it1 = array.begin();
  auto it2 = ret.begin();
  Type sum = Type(0);
  while (it1.is_valid())
  {
    *it2 = Exp(*it1 - array_max);
    sum += *it2;
    ++it2;
    ++it1;
  }

  auto it3 = ret.begin();  // TODO (private 855): Fix implictly deleted copy const. for iterator
  while (it3.is_valid())
  {
    *it3 /= sum;
    ++it3;
  }
}

template <typename ArrayType>
void Softmax2DImplementation(ArrayType const &array, ArrayType &ret,
                             typename ArrayType::SizeType axis)
{
  assert(ret.size() == array.size());
  assert(array.shape().size() == 2);
  assert(ret.shape().size() == 2);
  assert((axis == 0) || (axis == 1));

  for (std::size_t i = 0; i < array.shape()[axis]; ++i)
  {
    auto cur_slice = array.Slice(i, axis).Copy();
    auto ret_slice = ret.Slice(i, axis).Copy();
    Softmax1DImplementation(cur_slice, ret_slice);
    ret.Slice(i, axis).Assign(ret_slice);
  }
}
}  // namespace details

template <typename ArrayType>
void Softmax(ArrayType const &array, ArrayType &ret, typename ArrayType::SizeType axis)
{
  assert(ret.size() == array.size());

  if ((array.shape().size() == 1) && (ret.shape().size() == 1))
  {
    assert(axis == 0);
    details::Softmax1DImplementation(array, ret);
  }
  else if ((array.shape().size() == 2) && (ret.shape().size() == 2))
  {
    details::Softmax2DImplementation(array, ret, axis);
  }
  else if ((array.shape().size() == 3) && (ret.shape().size() == 3))
  {
    assert(axis == 0 || axis == 1);
    // TODO (#1320) - Copy()s can be removed when softmax2dimplementation handles IsIterable types
    auto tmp_ret = ret.View(0).Copy();
    for (size_t i = 0; i < array.shape()[2]; i++)
    {
      details::Softmax2DImplementation(array.View(i).Copy(), tmp_ret, axis);
      ret.View(i).Assign(tmp_ret);
    }
  }
  else if ((array.shape().size() == 3) && (ret.shape().size() == 3) && (axis == 2))
  {
    throw std::runtime_error("softmax on batch dimension is not implemented");
  }
  else
  {
    // TODO (#1360) we should have a N dimension implementation for softmax, with which the 2D
    // implementation should be merged.
    throw std::runtime_error("softmax for more than 3 Dimensions not yet handled");
  }
}

template <typename ArrayType>
void Softmax(ArrayType const &array, ArrayType &ret)
{
  assert(ret.size() == array.size());
  Softmax(array, ret, typename ArrayType::SizeType(0));
}

template <typename ArrayType>
ArrayType Softmax(ArrayType const &array, typename ArrayType::SizeType axis)
{
  ArrayType ret{array.shape()};
  Softmax(array, ret, axis);
  return ret;
}
template <typename ArrayType>
ArrayType Softmax(ArrayType const &array)
{
  ArrayType ret{array.shape()};
  Softmax(array, ret, 0);
  return ret;
}

}  // namespace math
}  // namespace fetch
