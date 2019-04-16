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
 * Computes the IEEE remainder of the floating point division operation x/y
 * @tparam type
 */
namespace fetch {
namespace math {

//////////////////////
/// Implemenations ///
//////////////////////

template <typename Type>
meta::IfIsNonFixedPointArithmetic<Type, void> Remainder(Type const &x, Type const &y, Type &ret)
{
  ret = std::remainder(x, y);
}

// TODO(800) - native implementations of fixed point are required; casting to double will not be
// permissible
template <typename T>
meta::IfIsFixedPoint<T, void> Remainder(T const &x, T const &y, T &ret)
{
  ret = T::Remainder(x, y);
}

//////////////////
/// Interfaces ///
//////////////////

template <typename Type>
meta::IfIsArithmetic<Type, Type> Remainder(Type const &x)
{
  Type ret;
  Log(x, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Remainder(ArrayType const &array1, ArrayType const &array2,
                                               ArrayType &ret)
{
  ASSERT(ret.shape() == array1.shape());
  ASSERT(ret.shape() == array2.shape());
  typename ArrayType::SizeType ret_count{0};
  for (typename ArrayType::Type &e : array1)
  {
    Remainder(e, array2.At(ret_count), ret.At(ret_count));
    ++ret_count;
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Remainder(ArrayType const &array1,
                                                    ArrayType const &array2)
{
  ASSERT(array2.shape() == array1.shape());
  ArrayType                    ret{array1.shape()};
  typename ArrayType::SizeType ret_count{0};
  for (typename ArrayType::Type &e : array1)
  {
    Remainder(e, array2.At(ret_count), ret.At(ret_count));
    ++ret_count;
  }
  return ret;
}

}  // namespace math
}  // namespace fetch