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

#include "math/kernels/standard_functions/abs.hpp"
#include "math/meta/math_type_traits.hpp"

/**
 * assigns the absolute of x to this array
 * @param x
 */

namespace fetch {
namespace math {

template <typename ArrayType>
fetch::math::meta::IfIsBlasArray<ArrayType, void> Abs(ArrayType &x, ArrayType &ret)
{
  math::free_functions::kernels::Abs<typename ArrayType::Type> kernel;
  ret.data().in_parallel().Apply(kernel, x.data());
}

template <typename ArrayType>
fetch::math::meta::IfIsNonBlasArray<ArrayType, void> Abs(ArrayType &x, ArrayType &ret)
{
  assert(x.size() == ret.size());
  for (std::size_t j = 0; j < ret.size(); ++j)
  {
    x.At(j) = std::abs(ret.At(j));
  }
}

template <typename Type>
fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> Abs(Type &x, Type &ret)
{
  ret = std::abs(x);
}

template <typename Type>
fetch::math::meta::IfIsFixedPoint<Type, void> Abs(Type &n, Type &ret)
{
  ret = n;
  if (ret < Type(0))
  {
    ret *= Type(-1);
  }
}

template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> Abs(ArrayType &x)
{
  ArrayType ret{x.shape()};
  Abs(x, ret);
  return ret;
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> Abs(Type &n)
{
  Type ret;
  Abs(n, ret);
  return ret;
}

//
// void Abs(fetch::fixed_point::FixedPoint<I, F> &n)
//{
//  if (n < fetch::fixed_point::FixedPoint<I, F>(0))
//  {
//    n *= fetch::fixed_point::FixedPoint<I, F>(-1);
//  }
//}

}  // namespace math
}  // namespace fetch