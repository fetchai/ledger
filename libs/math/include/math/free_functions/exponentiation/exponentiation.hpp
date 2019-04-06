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

/////////////////////////////////////
/// include specific math functions
/////////////////////////////////////

#include "math/free_functions/fundamental_operators.hpp"  // add, subtract etc.
#include "math/free_functions/standard_functions/exp.hpp"
#include "math/free_functions/standard_functions/log.hpp"

namespace fetch {
namespace math {

namespace details {

template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> SquareImpl(ArrayType const &x, ArrayType &ret)
{
  assert(x.size() == ret.size());
  auto it = x.begin();
  auto eit = x.end();
  auto rit = ret.begin();
  
  while(it != eit)
  {
    *rit = (*it) * (*it);
    ++it;
    ++rit;
  }

}

template <typename T>
fetch::math::meta::IfIsArithmetic<T, void> SquareImpl(T const &x, T &ret)
{
  ret = x * x;
}

}  // namespace details

/**
 * maps every element of the array x to x' = sqrt(x)
 * @param x - array
 */

template <typename ArrayType>
fetch::math::meta::IfIsNonBlasArray<ArrayType, void> Sqrt(ArrayType const &x, ArrayType &ret)
{
  for (std::size_t j = 0; j < x.size(); ++j)
  {
    ret.Set(j, static_cast<typename ArrayType::Type>(std::sqrt(static_cast<double>(x.At(j)))));
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsNonBlasArray<ArrayType, ArrayType> Sqrt(ArrayType const &x)
{
  ArrayType ret(x.shape());
  Sqrt(x, ret);
  return ret;
}

template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> Sqrt(ArrayType const &x, ArrayType &ret)
{
  for (std::size_t j = 0; j < x.size(); ++j)
  {
    ret.Set(j, static_cast<typename ArrayType::Type>(std::sqrt(static_cast<double>(x.At(j)))));
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, ArrayType> Sqrt(ArrayType const &x)
{
  ArrayType ret(x.shape());
  Sqrt(x, ret);
  return ret;
}

template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, void> Sqrt(Type const &x, Type &ret)
{
  ret = static_cast<Type>(std::sqrt(static_cast<double>(x)));
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> Sqrt(Type const &x)
{
  Type ret;
  Sqrt(x, ret);
  return ret;
}

/**
 * maps every element of the array x to x' = x^2
 * returns new array without altering input array
 * @param x - array
 */
template <typename T>
fetch::math::meta::IfIsArithmetic<T, void> Square(T const &x, T &ret)
{
  fetch::math::details::SquareImpl(x, ret);
}

template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> Square(ArrayType const &x, ArrayType &ret)
{
  fetch::math::details::SquareImpl(x, ret);
}
/**
 * maps every element of the array x to x' = x^2
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> Square(ArrayType &x)
{
  ArrayType ret(x.shape());
  Square(x, ret);
  return ret;
}

template <typename T>
fetch::math::meta::IfIsArithmetic<T, T> Square(T &x)
{
  T ret;
  Square(x, ret);
  return ret;
}


}  // namespace math
}  // namespace fetch
