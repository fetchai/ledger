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
//#include "math/kernels/standard_functions/log.hpp"
#include "math/meta/math_type_traits.hpp"

/**
 * natural logarithm of x
 * @param x
 */

namespace fetch {
namespace math {

template <typename ArrayType>
fetch::math::meta::IfIsMathShapeArray<ArrayType, ArrayType> Log(ArrayType const &x)
{
  ArrayType ret{x.shape()};
  ret.Copy(x);
  Log(ret);
  return ret;
}

template <typename Type>
meta::IfIsNonFixedPointArithmetic<Type, void> Log(Type const &x, Type &ret)
{
  ret = std::log(x);
}

template <typename Type>
meta::IfIsNonFixedPointArithmetic<Type, Type> Log(Type const &x)
{
  Type ret;
  Log(x, ret);
  return ret;
}

template <typename T>
meta::IfIsFixedPoint<T, void> Log(T const &n, T &ret)
{
  ret = T(std::log(double(n)));
}

//
template <typename T>
meta::IfIsFixedPoint<T, T> Log(T const &n)
{
  T ret = T(std::log(double(n)));
  return ret;
}

template <typename ArrayType>
meta::IfIsNonBlasArray<ArrayType, ArrayType> Log(ArrayType  &x)
{
  for (typename ArrayType::Type &e : x)
  {
    fetch::math::Log(e, e);
  }
  return x;
}
template <typename ArrayType>
meta::IfIsMathFixedPointArray<ArrayType, ArrayType> Log(ArrayType &x)
{
  for (typename ArrayType::Type &e : x)
  {
    fetch::math::Log(e, e);
  }
  return x;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Log(ArrayType const &array, ArrayType &ret)
{
  ASSERT(ret.shape() == array.shape());
  auto it1 = array.begin();
  auto it2 = ret.begin();
  auto eit1 = array.end();

  while(it1 != eit1)
  {
    Log(*it1, *it2);
    ++it1;
    ++it2;
  }
}

}  // namespace math
}  // namespace fetch