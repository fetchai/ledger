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

#include "core/fixed_point/fixed_point.hpp"
#include "math/kernels/standard_functions/trigonometric.hpp"
#include "math/meta/math_type_traits.hpp"

/**
 * tanh(x)
 * @param x
 */

namespace fetch {
namespace math {

template <typename T>
fetch::math::meta::IfIsArithmetic<T, void> Tanh(T const &x, T &ret)
{
  ret = T(std::tanh(static_cast<double>(x)));
}
template <typename T>
fetch::math::meta::IfIsArithmetic<T, T> Tanh(T const &x)
{
  T ret;
  Tanh(x, ret);
  return ret;
}

template <std::size_t I, std::size_t F>
void Tanh(fixed_point::FixedPoint<I, F> &x)
{
  x = fixed_point::FixedPoint<I, F>(std::tanh(double(x)));
}

template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Tanh(ArrayType &x)
{
  free_functions::kernels::Tanh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

template <typename ArrayType>
fetch::math::meta::IfIsNonBlasArray<ArrayType, void> Tanh(ArrayType &x)
{
  for (typename ArrayType::Type &e : x)
  {
    Tanh(e, e);
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> Tanh(ArrayType &x)
{
  for (typename ArrayType::Type &e : x)
  {
    Tanh(e, e);
  }
}

template <typename T>
fetch::math::meta::IfIsMathArray<T, void> Tanh(T const &array, T &ret)
{
  ret = array;
  for (typename T::Type &e : ret)
  {
    Tanh(e, e);
  }
}

}  // namespace math
}  // namespace fetch