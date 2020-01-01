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

#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/meta/math_type_traits.hpp"
#include "math/standard_functions/exp.hpp"

#include <cassert>
#include <cstddef>

namespace fetch {
namespace math {

namespace details {

/*
 * Really naive implementation that relies only on TensorType providing a At(std::size_t) method
 * TODO(private, 520) -- Clean up once we get unified TensorType + operations
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
  auto sum = Type(0);
  while (it1.is_valid())
  {
    *it2 = static_cast<Type>(Exp(*it1 - array_max));
    sum  = static_cast<Type>(sum + *it2);
    ++it2;
    ++it1;
  }

  auto it3 = ret.begin();  // TODO (private 855): Fix implicitly deleted copy const. for iterator
  while (it3.is_valid())
  {
    *it3 = static_cast<Type>(*it3 / sum);
    ++it3;
  }
}

/**
 * Any-D softmax implementation
 * @tparam ArrayType
 * @param array
 * @param ret
 * @param axis
 */
template <typename ArrayType>
void SoftmaxNDImplementation(ArrayType const &array, ArrayType &ret,
                             typename ArrayType::SizeType axis)
{
  // Subtract max for numerical stability
  ArrayType sums = ReduceMax(array, axis);
  Subtract(array, sums, ret);

  // exp(x)/sum(exp(x))
  Exp(ret, ret);
  ReduceSum(ret, axis, sums);
  Divide(ret, sums, ret);
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
  else
  {
    details::SoftmaxNDImplementation(array, ret, axis);
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

/**
 * Softmax implementation for multiple dimensions
 * @tparam ArrayType
 * @param array
 * @param ret
 * @param axes Vector of SizeType
 */
template <typename ArrayType>
void Softmax(ArrayType const &array, ArrayType &ret, std::vector<SizeType> axes)
{
  assert(ret.shape() == array.shape());
  assert(axes.size() >= 2);

  // Subtract max for numerical stability
  ArrayType sums = ReduceMax(array, axes);
  Subtract(array, sums, ret);

  // exp(x)/sum(exp(x))
  Exp(ret, ret);
  ReduceSum(ret, axes, sums);
  Divide(ret, sums, ret);
}

}  // namespace math
}  // namespace fetch
