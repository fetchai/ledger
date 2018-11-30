#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
#include "math/meta/type_traits.hpp"
#include <cassert>

namespace fetch {
namespace math {

/**
 * maps every element of the array x to x' = sin(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IsMathArrayLike<ArrayType, void> Sin(ArrayType &x)
{
  kernels::stdlib::Sin<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = cos(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IsMathArrayLike<ArrayType, void> Cos(ArrayType &x)
{
  kernels::stdlib::Cos<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = tan(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IsMathArrayLike<ArrayType, void> Tan(ArrayType &x)
{
  kernels::stdlib::Tan<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = arcsin(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IsMathArrayLike<ArrayType, void> Asin(ArrayType &x)
{
  kernels::stdlib::Asin<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = arccos(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IsMathArrayLike<ArrayType, void> Acos(ArrayType &x)
{
  kernels::stdlib::Acos<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = arctan(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IsMathArrayLike<ArrayType, void> Atan(ArrayType &x)
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
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Atan2(ArrayType &x)
{
  kernels::stdlib::Atan2<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = sinh(x), where sinh is the hyperbolic sine function
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IsMathArrayLike<ArrayType, void> Sinh(ArrayType &x)
{
  kernels::stdlib::Sinh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = cosh(x), where cosh is the hyperbolic cosine function
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IsMathArrayLike<ArrayType, void> Cosh(ArrayType &x)
{
  kernels::stdlib::Cosh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = tanh(x), where tanh is the hyperbolic tangent function
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IsMathArrayLike<ArrayType, void> Tanh(ArrayType &x)
{
  kernels::stdlib::Tanh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = asinh(x),, where asinh is the arc hyperbolic sine
 * (inverse hyperbolic sine) of a number in radians.
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IsMathArrayLike<ArrayType, void> Asinh(ArrayType &x)
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
fetch::math::meta::IsMathArrayLike<ArrayType, void> Acosh(ArrayType &x)
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
fetch::math::meta::IsMathArrayLike<ArrayType, void> Atanh(ArrayType &x)
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
void Hypot(ArrayType &x)
{
  kernels::stdlib::Hypot<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

}  // namespace math
}  // namespace fetch