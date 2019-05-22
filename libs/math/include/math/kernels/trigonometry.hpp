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

#include "core/assert.hpp"
#include <cmath>

#include "math/meta/math_type_traits.hpp"

namespace fetch {
namespace math {
namespace kernels {

struct Sin
{
  template <typename Type>
  fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> operator()(Type const &x,
                                                                        Type &      y) const
  {
    y = std::sin(x);
  }

  template <typename Type>
  fetch::math::meta::IfIsFixedPoint<Type, void> operator()(Type const &x, Type &y) const
  {
    y = Type::Sin(x);
  }
};

struct Cos
{
  template <typename Type>
  fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> operator()(Type const &x,
                                                                        Type &      y) const
  {
    y = std::cos(x);
  }

  template <typename Type>
  fetch::math::meta::IfIsFixedPoint<Type, void> operator()(Type const &x, Type &y) const
  {
    y = Type::Cos(x);
  }
};

struct Tan
{
  template <typename Type>
  fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> operator()(Type const &x,
                                                                        Type &      y) const
  {
    y = std::tan(x);
  }

  template <typename Type>
  fetch::math::meta::IfIsFixedPoint<Type, void> operator()(Type const &x, Type &y) const
  {
    y = Type::Tan(x);
  }
};

struct ASin
{
  template <typename Type>
  fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> operator()(Type const &x,
                                                                        Type &      y) const
  {
    y = std::asin(x);
  }

  template <typename Type>
  fetch::math::meta::IfIsFixedPoint<Type, void> operator()(Type const &x, Type &y) const
  {
    y = Type::ASin(x);
  }
};

struct ACos
{
  template <typename Type>
  fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> operator()(Type const &x,
                                                                        Type &      y) const
  {
    y = std::acos(x);
  }

  template <typename Type>
  fetch::math::meta::IfIsFixedPoint<Type, void> operator()(Type const &x, Type &y) const
  {
    y = Type::ACos(x);
  }
};

struct ATan
{
  template <typename Type>
  fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> operator()(Type const &x,
                                                                        Type &      y) const
  {
    y = std::atan(x);
  }

  template <typename Type>
  fetch::math::meta::IfIsFixedPoint<Type, void> operator()(Type const &x, Type &y) const
  {
    y = Type::ATan(x);
  }
};

struct ATan2
{
  template <typename Type>
  fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> operator()(Type const &x,
                                                                        Type const &y,
                                                                        Type &      z) const
  {
    z = std::atan2(x, y);
  }

  template <typename Type>
  fetch::math::meta::IfIsFixedPoint<Type, void> operator()(Type const &x, Type const &y,
                                                           Type &z) const
  {
    z = Type::ATan2(x, y);
  }
};

struct SinH
{
  template <typename Type>
  fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> operator()(Type const &x,
                                                                        Type &      y) const
  {
    y = std::sinh(x);
  }

  template <typename Type>
  fetch::math::meta::IfIsFixedPoint<Type, void> operator()(Type const &x, Type &y) const
  {
    y = Type::SinH(x);
  }
};

struct CosH
{
  template <typename Type>
  fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> operator()(Type const &x,
                                                                        Type &      y) const
  {
    y = std::cosh(x);
  }

  template <typename Type>
  fetch::math::meta::IfIsFixedPoint<Type, void> operator()(Type const &x, Type &y) const
  {
    y = Type::CosH(x);
  }
};

struct TanH
{
  template <typename Type>
  fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> operator()(Type const &x,
                                                                        Type &      y) const
  {
    y = std::tanh(x);
  }

  template <typename Type>
  fetch::math::meta::IfIsFixedPoint<Type, void> operator()(Type const &x, Type &y) const
  {
    y = Type::TanH(x);
  }
};

struct ASinH
{
  template <typename Type>
  fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> operator()(Type const &x,
                                                                        Type &      y) const
  {
    y = std::asinh(x);
  }

  template <typename Type>
  fetch::math::meta::IfIsFixedPoint<Type, void> operator()(Type const &x, Type &y) const
  {
    y = Type::ASinH(x);
  }
};

struct ACosH
{
  template <typename Type>
  fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> operator()(Type const &x,
                                                                        Type &      y) const
  {
    y = std::acosh(x);
  }

  template <typename Type>
  fetch::math::meta::IfIsFixedPoint<Type, void> operator()(Type const &x, Type &y) const
  {
    y = Type::ACosH(x);
  }
};

struct ATanH
{
  template <typename Type>
  fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> operator()(Type const &x,
                                                                        Type &      y) const
  {
    y = std::atanh(x);
  }

  template <typename Type>
  fetch::math::meta::IfIsFixedPoint<Type, void> operator()(Type const &x, Type &y) const
  {
    y = Type::ATanH(x);
  }
};

}  // namespace kernels
}  // namespace math
}  // namespace fetch
