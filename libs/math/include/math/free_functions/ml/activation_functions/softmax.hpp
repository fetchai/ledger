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

#include "math/kernels/approx_logistic.hpp"
#include "math/kernels/basic_arithmetics.hpp"
#include "math/kernels/relu.hpp"
#include "math/kernels/standard_functions.hpp"

#include "core/assert.hpp"
#include "math/meta/math_type_traits.hpp"
#include "vectorise/memory/range.hpp"
#include <algorithm>
#include <cassert>
#include <numeric>
#include <vector>

#include "math/free_functions/fundamental_operators.hpp"  // add, subtract etc.
#include "math/free_functions/standard_functions/abs.hpp"
#include "math/free_functions/standard_functions/exp.hpp"
#include "math/free_functions/standard_functions/fmod.hpp"
#include "math/free_functions/standard_functions/remainder.hpp"

#include "math/free_functions/comparison/comparison.hpp"
#include "math/free_functions/exponentiation/exponentiation.hpp"
#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "math/meta/math_type_traits.hpp"

namespace fetch {
namespace math {

namespace details {

/*
 * Really naive implementation that relies only on ArrayType providing a At(std::size_t) method
 * TODO(private, 520) -- Clean up once we get unified ArrayType + operations
 */

template <typename ArrayType>
void Softmax1DImplementation(ArrayType const &array, ArrayType &ret)
{
  assert(ret.size() == array.size());
  assert(array.shape().size() == 1);
  assert(ret.shape().size() == 1);

  // subtract max for numerical stability
  typename ArrayType::Type array_max = std::numeric_limits<typename ArrayType::Type>::lowest();
  Max(array, array_max);
  Subtract(array, array_max, ret);

  // softmax (Exp(x) / Sum(Exp(x)))
  Exp(ret, ret);
  typename ArrayType::Type array_sum = typename ArrayType::Type(0);
  Sum(ret, array_sum);
  Divide(ret, array_sum, ret);
}

template <typename ArrayType>
void Softmax2DImplementation(ArrayType const &array, ArrayType &ret,
                             typename ArrayType::SizeType axis)
{
  assert(ret.size() == array.size());
  assert(array.shape().size() == 2);
  assert(ret.shape().size() == 2);
  assert((axis == 0) || (axis == 1));
  assert(axis == 0);  // arbitrary dimension slicing not yet implemented

  for (std::size_t i = 0; i < array.shape()[axis]; ++i)
  {
    ArrayType cur_slice = array.Slice(i);
    ArrayType ret_slice = ret.Slice(i);
    Softmax1DImplementation(cur_slice, ret_slice);
    ret.Slice(i).Copy(ret_slice);
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
  else
  {
    throw std::runtime_error("softmax for nDimensions not yet handled");
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
