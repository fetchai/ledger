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
 *
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IfIsNotImplemented<ArrayType, void> Isgreater(ArrayType &x)
{
  kernels::stdlib::Isgreater<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Isgreaterequal(ArrayType const &x, ArrayType const &y, ArrayType &z)
{
  kernels::stdlib::Isgreaterequal<typename ArrayType::Type> kernel;
  z.data().in_parallel().Apply(kernel, x.data(), y.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IfIsNotImplemented<ArrayType, void> Isless(ArrayType &x)
{
  kernels::stdlib::Isless<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IfIsNotImplemented<ArrayType, void> Islessequal(ArrayType &x)
{
  kernels::stdlib::Islessequal<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IfIsNotImplemented<ArrayType, void> Islessgreater(ArrayType &x)
{
  kernels::stdlib::Islessgreater<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Determines if the floating point numbers x and y are unordered, that is, one or both are NaN and
 * thus cannot be meaningfully compared with each other.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IfIsNotImplemented<ArrayType, void> Isunordered(ArrayType &x)
{
  kernels::stdlib::Isunordered<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Max function for two values
 * @tparam T
 * @param datum1
 * @param datum2
 * @return
 */
template <typename T>
T Max(T const &datum1, T const &datum2, T &ret)
{
  ret = std::max(datum1, datum2);
  return ret;
}
template <typename T>
T Max(T const &datum1, T const &datum2)
{
  T ret{};
  ret = Max(datum1, datum2, ret);
  return ret;
}

/**
 * Min function for two values
 * @tparam T
 * @param datum1
 * @param datum2
 * @return
 */
template <typename T>
inline void Min(T const &datum1, T const &datum2, T &ret)
{
  ret = std::min(datum1, datum2);
}
template <typename T>
inline T Min(T const &datum1, T const &datum2)
{
  T ret = std::min(datum1, datum2);
  return ret;
}

}  // namespace math
}  // namespace fetch