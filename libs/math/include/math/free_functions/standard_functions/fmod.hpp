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

#include "math/meta/math_type_traits.hpp"

/**
 * Computes the floating-point remainder of the division operation x / y
 */
namespace fetch {
namespace math {

template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, void> Fmod(Type const &x, Type const &y, Type &z)
{
  z = std::fmod(x, y);
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, void> Fmod(Type const &x, Type &y)
{
  y = std::fmod(x, y);
}

}  // namespace math
}  // namespace fetch
