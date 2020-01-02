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

#include "math/meta/math_type_traits.hpp"

#include <cassert>

/**
 * natural logarithm of x
 * @param x
 */

namespace fetch {
namespace math {

///////////////////////
/// IMPLEMENTATIONS ///
///////////////////////

template <typename Type>
fetch::meta::EnableIf<fetch::meta::IsInteger<Type> || fetch::meta::IsFloat<Type>, void> Log(
    Type const &x, Type &ret)
{
  ret = static_cast<Type>(std::log(x));
}

template <typename T>
meta::IfIsFixedPoint<T, void> Log(T const &n, T &ret)
{
  ret = T::Log(n);
}

template <typename Type>
fetch::meta::EnableIf<fetch::meta::IsInteger<Type> || fetch::meta::IsFloat<Type>, void> Log2(
    Type const &x, Type &ret)
{
  ret = std::log2(x);
}

template <typename T>
meta::IfIsFixedPoint<T, void> Log2(T const &n, T &ret)
{
  ret = T::Log2(n);
}

template <typename Type>
fetch::meta::EnableIf<fetch::meta::IsInteger<Type> || fetch::meta::IsFloat<Type>, void> Log10(
    Type const &x, Type &ret)
{
  ret = std::log10(x);
}

template <typename T>
meta::IfIsFixedPoint<T, void> Log10(T const &n, T &ret)
{
  ret = T::Log10(n);
}

//////////////////
/// INTERFACES ///
//////////////////

template <typename Type>
meta::IfIsArithmetic<Type, Type> Log(Type const &x)
{
  Type ret;
  Log(x, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Log(ArrayType const &array, ArrayType &ret)
{
  assert(ret.shape() == array.shape());
  auto it1 = array.cbegin();
  auto rit = ret.begin();
  while (it1.is_valid())
  {
    Log(*it1, *rit);
    ++it1;
    ++rit;
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Log(ArrayType const &array)
{
  ArrayType ret{array.shape()};
  Log(array, ret);
  return ret;
}

template <typename Type>
meta::IfIsArithmetic<Type, Type> Log2(Type const &x)
{
  Type ret;
  Log2(x, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Log2(ArrayType const &array, ArrayType &ret)
{
  assert(ret.shape() == array.shape());
  auto it1 = array.cbegin();
  auto rit = ret.begin();
  while (it1.is_valid())
  {
    Log2(*it1, *rit);
    ++it1;
    ++rit;
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Log2(ArrayType const &array)
{
  ArrayType ret{array.shape()};
  Log2(array, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Log10(ArrayType const &array, ArrayType &ret)
{
  assert(ret.shape() == array.shape());
  auto it1 = array.cbegin();
  auto rit = ret.begin();
  while (it1.is_valid())
  {
    Log10(*it1, *rit);
    ++it1;
    ++rit;
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Log10(ArrayType const &array)
{
  ArrayType ret{array.shape()};
  Log10(array, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
