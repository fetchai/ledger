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

#include <cassert>

#include <cfenv>
#include <cmath>

#pragma STDC FENV_ACCESS ON

namespace fetch {
namespace math {

///////////////////////
/// IMPLEMENTATIONS ///
///////////////////////

template <typename Type>
meta::IfIsNonFixedPointArithmetic<Type, void> Sqrt(Type const &x, Type &ret)
{
  ret = Type(std::sqrt(x));
}

// TODO(800) - native implementations of fixed point are required; casting to double will not be
// permissible
template <typename T>
meta::IfIsFixedPoint<T, void> Sqrt(T const &n, T &ret)
{
  ret = T::Sqrt(n);
}

//////////////////
/// INTERFACES ///
//////////////////

template <typename Type>
meta::IfIsArithmetic<Type, Type> Sqrt(Type const &x)
{
  Type ret;
  Sqrt(x, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Sqrt(ArrayType const &array, ArrayType &ret)
{
  assert(ret.shape() == array.shape());
  auto arr_it = array.cbegin();
  auto rit    = ret.begin();

  std::feclearexcept(FE_ALL_EXCEPT);
  while (arr_it.is_valid())
  {
    Sqrt(*arr_it, *rit);
    ++arr_it;
    ++rit;
  }
  if(std::fetestexcept(FE_INVALID)) {
    throw std::runtime_error("Sqrt error");
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Sqrt(ArrayType const &array)
{
  ArrayType ret{array.shape()};
  Sqrt(array, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
