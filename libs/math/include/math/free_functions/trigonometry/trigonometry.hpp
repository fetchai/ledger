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

#include "math/kernels/standard_functions.hpp"
#include "math/meta/math_type_traits.hpp"
#include <cassert>

namespace fetch {
namespace math {

/**
 * maps every element of the array x to x' = sin(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Sin(ArrayType &x)
{
  kernels::stdlib::Sin<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = cos(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Cos(ArrayType &x)
{
  kernels::stdlib::Cos<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = tan(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Tan(ArrayType &x)
{
  kernels::stdlib::Tan<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = arcsin(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Asin(ArrayType &x)
{
  kernels::stdlib::Asin<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = arccos(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Acos(ArrayType &x)
{
  kernels::stdlib::Acos<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = arctan(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Atan(ArrayType &x)
{
  kernels::stdlib::Atan<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * The atan2() function in C++ returns the inverse tangent of a coordinate in radians.
 * @param y: this value represents the proportion of the y-coordinate.
 * @param x: this value represents the proportion of the x-coordinate
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Atan2(ArrayType &x)
{
  kernels::stdlib::Atan2<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = sinh(x), where sinh is the hyperbolic sine function
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Sinh(ArrayType &x)
{
  kernels::stdlib::Sinh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = cosh(x), where cosh is the hyperbolic cosine function
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Cosh(ArrayType &x)
{
  kernels::stdlib::Cosh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = tanh(x), where tanh is the hyperbolic tangent function
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Tanh(ArrayType &x)
{
  kernels::stdlib::Tanh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename ArrayType>
fetch::math::meta::IfIsNonBlasArray<ArrayType, ArrayType> Tanh(ArrayType const &x)
{
  ArrayType                    ret(x);
  typename ArrayType::SizeType idx{0};
  for (typename ArrayType::Type &e : x)
  {
    ret.At(idx) = std::tanh(e);
    idx++;
  }

  return ret;
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, ArrayType> Tanh(ArrayType const &x)
{
  ArrayType                    ret{x.shape()};
  typename ArrayType::SizeType idx{0};
  for (typename ArrayType::Type &e : x)
  {
    ret.At(idx) = static_cast<typename ArrayType::Type>(std::tanh(double(e)));
    idx++;
  }

  return ret;
}
template <typename T>
fetch::math::meta::IfIsArithmetic<T, void> Tanh(T const &x, T &ret)
{
  ret = T(std::tanh(static_cast<double>(x)));
}
template <typename T>
fetch::math::meta::IfIsArithmetic<T, T> Tanh(T const &x)
{
  T ret;
  ret = std::tanh(x);
  return ret;
}
template <std::size_t I, std::size_t F>
fixed_point::FixedPoint<I, F> Tanh(fixed_point::FixedPoint<I, F> &x)
{
  fixed_point::FixedPoint<I, F> ret(std::tanh(double(x)));
  return ret;
}
template <typename T>
fetch::math::meta::IfIsMathArray<T, void> Tanh(T const &array, T &ret)
{
  // ret = array;
  // for (typename T::Type &e : ret)
  // {
  //   e = static_cast<typename T::Type>(std::tanh(double(e)));
  // }

  assert(ret.shape() == array.shape());
  typename T::SizeType idx{0};
  for (auto &e : array)
  {
    Tanh(e, ret.At(idx));
    idx++;
  }
}

/**
 * maps every element of the array x to x' = asinh(x),, where asinh is the arc hyperbolic sine
 * (inverse hyperbolic sine) of a number in radians.
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Asinh(ArrayType &x)
{
  kernels::stdlib::Asinh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = acosh(x), where acosh is the arc hyperbolic cosine
 * (inverse hyperbolic cosine) of a number in radians.
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Acosh(ArrayType &x)
{
  kernels::stdlib::Acosh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = atanh(x), where atanh arc hyperbolic tangent (inverse
 * hyperbolic tangent) of a number in radians.
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Atanh(ArrayType &x)
{
  kernels::stdlib::Atanh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * If no errors occur and there are two inputs, the hypotenuse of a right-angled triangle is
 * computed as sqrt(x^2 + y^2) If no errors occur and there are 3 points, then the distance from the
 * origin in 3D space is returned as sqrt(x^2 + y^2 + z^2)
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Hypot(ArrayType &x)
{
  kernels::stdlib::Hypot<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

}  // namespace math
}  // namespace fetch