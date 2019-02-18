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

#include "math/kernels/standard_functions/exp.hpp"
#include "math/meta/math_type_traits.hpp"

/**
 * e^x
 * @param x
 */

namespace fetch {
namespace math {

template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> Exp(ArrayType &x)
{
  free_functions::kernels::Exp<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, void> Exp(Type &x)
{
  x = std::exp(x);
}

template <std::size_t I, std::size_t F>
void Exp(fetch::fixed_point::FixedPoint<I, F> &x)
{
  x = fetch::fixed_point::FixedPoint<I, F>(std::exp(double(x)));
}

}  // namespace math
}  // namespace fetch