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
 * finite check
 * @param x
 */
template <typename ArrayType>
void Isfinite(ArrayType &x)
{
  kernels::stdlib::Isfinite<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * checks for inf values
 * @param x
 */
template <typename ArrayType>
void Isinf(ArrayType &x)
{
  kernels::stdlib::Isinf<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * checks for nans
 * @param x
 */
template <typename ArrayType>
void Isnan(ArrayType &x)
{
  kernels::stdlib::Isnan<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Categorizes floating point value arg into the following categories: zero, subnormal, normal,
 * infinite, NAN, or implementation-defined category.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IfIsNotImplemented<ArrayType, void> Fpclassify(ArrayType &x)
{
  kernels::stdlib::Fpclassify<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Determines if the given floating point number arg is normal, i.e. is neither zero, subnormal,
 * infinite, nor NaN.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IfIsNotImplemented<ArrayType, void> Isnormal(ArrayType &x)
{
  kernels::stdlib::Isnormal<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

}  // namespace math
}  // namespace fetch
