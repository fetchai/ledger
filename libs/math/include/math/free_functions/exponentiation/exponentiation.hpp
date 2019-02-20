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

#include "math/kernels/approx_exp.hpp"
#include "math/kernels/approx_log.hpp"
#include "math/kernels/standard_functions.hpp"

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
void SquareImpl(ArrayType const &x, ArrayType &ret)
{
  assert(x.size() == ret.size());
  for (std::size_t i = 0; i < x.size(); ++i)
  {
    ret[i] = x[i] * x[i];
  }
}
}  // namespace details
/**
 * maps every element of the array x to x' = 2^x
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> Exp2(ArrayType &x)
{
  kernels::stdlib::Exp2<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = exp(x) - 1
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> Expm1(ArrayType &x)
{
  kernels::stdlib::Expm1<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = log(x) - 1, where log uses base 10
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> Log10(ArrayType &x)
{
  kernels::stdlib::Log10<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = log(x) - 1, where log uses base 2
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> Log2(ArrayType &x)
{
  kernels::stdlib::Log2<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = ln(1 + x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> Log1p(ArrayType &x)
{
  kernels::stdlib::Log1p<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = sqrt(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Sqrt(ArrayType &x)
{
  kernels::stdlib::Sqrt<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename ArrayType>
fetch::math::meta::IfIsNonBlasArray<ArrayType, void> Sqrt(ArrayType &x)
{
  for (std::size_t j = 0; j < x.size(); ++j)
  {
    x.Set(j, static_cast<typename ArrayType::Type>(std::sqrt(static_cast<double>(x.At(j)))));
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> Sqrt(ArrayType &x)
{
  for (std::size_t j = 0; j < x.size(); ++j)
  {
    x.Set(j, static_cast<typename ArrayType::Type>(std::sqrt(static_cast<double>(x.At(j)))));
  }
}

/**
 * maps every element of the array x to x' = cubic_root(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> Cbrt(ArrayType &x)
{
  kernels::stdlib::Cbrt<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = x^y
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsNotImplemented<ArrayType, void> Pow(ArrayType &x)
{
  kernels::stdlib::Pow<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = x^2
 * @param x - array
 */
template <typename T>
void Square(T &x)
{
  fetch::math::details::SquareImpl(x, x);
}

/**
 * maps every element of the array x to x' = x^2
 * returns new array without altering input array
 * @param x - array
 */
template <typename T>
void Square(T const &x, T &ret)
{
  fetch::math::details::SquareImpl(x, ret);
}

/**
 * maps every element of the array x to x' = e^x
 * implemented with greater speed but less accuracy
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> ApproxExp(ArrayType &x)
{
  kernels::ApproxExp<typename ArrayType::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * maps every element of the array x to x' = log(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> ApproxLog(ArrayType &x)
{
  kernels::ApproxLog<typename ArrayType::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

}  // namespace math
}  // namespace fetch
