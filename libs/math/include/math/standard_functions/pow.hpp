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

namespace fetch {
namespace math {

//////////////////////
/// Implemenations ///
//////////////////////

template <typename Type>
meta::IfIsNonFixedPointArithmetic<Type, void> Pow(Type const &x, Type const &y, Type &ret)
{
  ret = Type(std::pow(x, y));
}

// TODO(800) - native implementations of fixed point are required; casting to double will not be
// permissible
template <typename T>
meta::IfIsFixedPoint<T, void> Pow(T const &x, T const &y, T &ret)
{
  ret = T::Pow(x, y);
}

//////////////////
/// Interfaces ///
//////////////////

template <typename Type>
meta::IfIsArithmetic<Type, Type> Pow(Type const &x, Type const &exponent)
{
  Type ret;
  Pow(x, exponent, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Pow(ArrayType const &               array1,
                                         typename ArrayType::Type const &exponent, ArrayType &ret)
{
  ASSERT(ret.shape() == array1.shape());
  typename ArrayType::SizeType ret_count{0};
  for (typename ArrayType::Type &e : array1)
  {
    Pow(e, exponent, ret.At(ret_count));
    ++ret_count;
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Pow(ArrayType const &               array1,
                                              typename ArrayType::Type const &exponent)
{
  ArrayType                    ret{array1.shape()};
  typename ArrayType::SizeType ret_count{0};
  for (typename ArrayType::Type &e : array1)
  {
    Pow(e, exponent, ret.At(ret_count));
    ++ret_count;
  }
  return ret;
}

template <typename Type>
meta::IfIsArithmetic<Type, Type> Square(Type const &x)
{
  Type ret;
  Pow(x, Type(2), ret);
  return ret;
}

template <typename Type>
meta::IfIsArithmetic<Type, void> Square(Type const &x, Type &ret)
{
  Pow(x, Type(2), ret);
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Square(ArrayType const &x)
{
  ArrayType ret{x.shape()};
  Pow(x, typename ArrayType::Type(2), ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Square(ArrayType const &x, ArrayType &ret)
{
  Pow(x, typename ArrayType::Type(2), ret);
}

}  // namespace math
}  // namespace fetch
