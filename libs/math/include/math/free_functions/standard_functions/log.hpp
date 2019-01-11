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

#include "math/kernels/standard_functions/log.hpp"
#include "math/meta/math_type_traits.hpp"

/**
 * natural logarithm of x
 * @param x
 */

namespace fetch {
namespace math {

template <typename ArrayType>
fetch::math::meta::IfIsMathShapeArray<ArrayType, void> Log(ArrayType &x)
{
  fetch::math::free_functions::kernels::Log<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename ArrayType>
fetch::math::meta::IfIsMathShapeArray<ArrayType, ArrayType> Log(ArrayType const &x)
{
  ArrayType ret{x.shape()};
  ret.Copy(x);
  Log(ret);
  return ret;
}
template <typename ArrayType>
fetch::math::meta::IfIsMathShapelessArray<ArrayType, ArrayType> Log(ArrayType const &x)
{
  ArrayType ret{x.size()};
  ret.Copy(x);
  Log(ret);
  return ret;
}

template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, void> Log(Type &x)
{
  x = std::log(x);
}

}  // namespace math
}  // namespace fetch