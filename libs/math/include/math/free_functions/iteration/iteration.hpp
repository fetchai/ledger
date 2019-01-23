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

#include <cassert>

#include "math/kernels/standard_functions.hpp"
#include "math/meta/math_type_traits.hpp"

namespace fetch {
namespace math {

/**
 * Returns the next representable value of from in the direction of to.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IfIsNotImplemented<ArrayType, void> Nextafter(ArrayType &x)
{
  kernels::stdlib::Nextafter<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Returns the next representable value of from in the direction of to. If from equals to to, to is
 * returned, converted from long double to the return type of the function without loss of range or
 * precision.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IfIsNotImplemented<ArrayType, void> Nexttoward(ArrayType &x)
{
  kernels::stdlib::Nexttoward<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

}  // namespace math
}  // namespace fetch
