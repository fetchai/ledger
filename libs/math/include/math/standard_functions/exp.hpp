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
#include "core/fixed_point/fixed_point.hpp"
#include "math/meta/math_type_traits.hpp"

/**
 * e^x
 * @param x
 */

namespace fetch {
namespace math {

///////////////////////
/// IMPLEMENTATIONS ///
///////////////////////

template <typename Type>
meta::IfIsNonFixedPointArithmetic<Type, void> Exp(Type const &x, Type &ret)
{
  ret = std::exp(x);
}

// TODO(800) - native implementations of fixed point are required; casting to double will not be
// permissible
template <typename T>
meta::IfIsFixedPoint<T, void> Exp(T const &n, T &ret)
{
  ret = T(std::exp(double(n)));
}

//////////////////
/// INTERFACES ///
//////////////////

template <typename Type>
meta::IfIsArithmetic<Type, Type> Exp(Type const &x)
{
  Type ret;
  Exp(x, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Exp(ArrayType const &array, ArrayType &ret)
{
  ASSERT(ret.shape() == array.shape());
  typename ArrayType::SizeType ret_count{0};
  for (typename ArrayType::Type &e : array)
  {
    Exp(e, ret.At(ret_count));
    ++ret_count;
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Exp(ArrayType const &array)
{
  ArrayType                    ret{array.shape()};
  typename ArrayType::SizeType ret_count{0};
  for (typename ArrayType::Type &e : array)
  {
    Exp(e, ret.At(ret_count));
    ++ret_count;
  }
  return ret;
}

}  // namespace math
}  // namespace fetch