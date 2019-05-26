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
#include "math/meta/math_type_traits.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

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
  ret = static_cast<Type>(std::exp(static_cast<double>(x)));
}

// TODO(800) - native implementations of fixed point are required; casting to double will not be
// permissible
template <typename T>
meta::IfIsFixedPoint<T, void> Exp(T const &n, T &ret)
{
  ret = T::Exp(n);
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
  auto it1 = array.cbegin();
  auto rit = ret.begin();
  while (it1.is_valid())
  {
    Exp(*it1, *rit);
    ++it1;
    ++rit;
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Exp(ArrayType const &array)
{
  ArrayType ret{array.shape()};
  Exp(array, ret);
  return ret;
  return ret;
}

}  // namespace math
}  // namespace fetch