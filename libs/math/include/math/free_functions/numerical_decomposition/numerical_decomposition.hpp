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
#include <cassert>
#include "math/meta/type_traits.hpp"

namespace fetch {
namespace math {

/**
 * Decomposes given floating point value arg into a normalized fraction and an integral power of
 * two.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Frexp(ArrayType &x)
{
  kernels::stdlib::Frexp<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Multiplies a floating point value x by the number 2 raised to the exp power.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Ldexp(ArrayType &x)
{
  kernels::stdlib::Ldexp<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Decomposes given floating point value x into integral and fractional parts, each having the same
 * type and sign as x. The integral part (in floating-point format) is stored in the object pointed
 * to by iptr.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Modf(ArrayType &x)
{
  kernels::stdlib::Modf<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Multiplies a floating point value x by FLT_RADIX raised to power exp.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Scalbn(ArrayType &x)
{
  kernels::stdlib::Scalbn<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Multiplies a floating point value x by FLT_RADIX raised to power exp.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Scalbln(ArrayType &x)
{
  kernels::stdlib::Scalbln<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Extracts the value of the unbiased exponent from the floating-point argument arg, and returns it
 * as a signed integer value.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Ilogb(ArrayType &x)
{
  kernels::stdlib::Ilogb<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Extracts the value of the unbiased radix-independent exponent from the floating-point argument
 * arg, and returns it as a floating-point value.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Logb(ArrayType &x)
{
  kernels::stdlib::Logb<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

} // math
} // fetch