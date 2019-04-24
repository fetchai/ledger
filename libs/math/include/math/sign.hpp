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

#include "math/kernels/sign.hpp"
#include "math/kernels/standard_functions.hpp"
#include "math/meta/math_type_traits.hpp"
#include <cassert>

namespace fetch {
namespace math {

/**
 * Composes a floating point value with the magnitude of x and the sign of y.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IfIsNotImplemented<ArrayType, void> Copysign(ArrayType &x)
{
  kernels::stdlib::Copysign<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Determines if the given floating point number arg is negative.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IfIsNotImplemented<ArrayType, void> Signbit(ArrayType &x)
{
  kernels::stdlib::Signbit<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * replaces data with the sign (1, 0, or -1)
 * @param x
 */
template <typename ArrayType>
void Sign(ArrayType &x)
{
  kernels::Sign<typename ArrayType::VectorRegisterType> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

}  // namespace math
}  // namespace fetch