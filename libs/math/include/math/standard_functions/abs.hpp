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

#include "math/meta/math_type_traits.hpp"

/**
 * assigns the absolute of x to this array
 * @param x
 */

namespace fetch {
namespace math {

///////////////////////
/// IMPLEMENTATIONS ///
///////////////////////

// this also handles case of passing uint to abs
template <typename T>
meta::IfIsArithmetic<T, void> Abs(T const &n, T &ret)
{
  ret = T(std::abs((double)n));
}

// TODO: this should be somehow reinstated, as current Abs does a cast to double
// template <typename T>
// meta::IsFixedPoint<T, void> Abs(T const &n, T &ret)
// {
//   ret = T::Abs(n);
// }

//////////////////
/// INTERFACES ///
//////////////////

template <typename Type>
meta::IfIsArithmetic<Type, Type> Abs(Type const &x)
{
  Type ret;
  Abs(x, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Abs(ArrayType const &array, ArrayType &ret)
{
  ASSERT(ret.shape() == array.shape());
  auto it1 = array.cbegin();
  auto rit = ret.begin();
  while (it1.is_valid())
  {
    Abs(*it1, *rit);
    ++it1;
    ++rit;
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Abs(ArrayType const &array)
{
  ArrayType ret{array.shape()};
  Abs(array, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch