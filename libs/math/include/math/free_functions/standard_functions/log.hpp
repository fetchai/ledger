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

#include "math/kernels/standard_functions/log.hpp"
#include "math/meta/math_type_traits.hpp"

/**
 * natural logarithm of x
 * @param x
 */

namespace fetch {
namespace math {

template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Log(ArrayType &x)
{
  fetch::math::free_functions::kernels::Log<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

template <typename ArrayType>
fetch::math::meta::IfIsMathShapeArray<ArrayType, ArrayType> Log(ArrayType const &x)
{
  ArrayType ret{x.shape()};
  ret.Copy(x);
  Log(ret);
  return ret;
}

template <typename Type>
meta::IfIsNonFixedPointArithmetic<Type, void> Log(Type &x, Type &ret)
{
  ret = std::log(x);
}

template <typename Type>
meta::IfIsNonFixedPointArithmetic<Type, void> Log(Type &x)
{
  Log(x, x);
}

//
template <typename T>
meta::IfIsFixedPoint<T, void> Log(T &n)
{
  n = T(std::log(double(n)));
}

template <typename ArrayType>
meta::IfIsNonBlasArray<ArrayType, void> Log(ArrayType &x)
{
  for (typename ArrayType::Type &e : x)
  {
    fetch::math::Log(e);
  }
}
template <typename ArrayType>
meta::IfIsMathFixedPointArray<ArrayType, void> Log(ArrayType &x)
{
  for (typename ArrayType::Type &e : x)
  {
    fetch::math::Log(e);
  }
}

template <typename T>
meta::IfIsMathArray<T, void> Log(T const &array, T &ret)
{
  ret = array;
  for (typename T::Type &e : ret)
  {
    Log(e);
  }
}

}  // namespace math
}  // namespace fetch