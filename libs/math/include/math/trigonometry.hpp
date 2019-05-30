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
#include "math/kernels/trigonometry.hpp"
#include "math/meta/math_type_traits.hpp"

namespace fetch {
namespace math {

/**
 * maps every element of the array x to ret = sin(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathNonFixedPointArray<ArrayType, void> Sin(ArrayType const &x,
                                                                   ArrayType &      ret)
{
  assert(ret.size() == x.size());
  kernels::Sin<typename ArrayType::Type> s;
  auto                                   x_it = x.cbegin();
  auto                                   rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> Sin(ArrayType const &x, ArrayType &ret)
{
  using Type = typename ArrayType::Type;
  assert(ret.size() == x.size());
  kernels::Sin<double> s;
  auto                 x_it = x.cbegin();
  auto                 rit  = ret.begin();
  double               casted_x;
  double               casted_ret;
  while (x_it.is_valid())
  {
    // TODO(800) - native fixed point implementation required - casting to double will not be
    // allowed
    casted_x   = double(*x_it);
    casted_ret = double(*rit);
    s(casted_x, casted_ret);
    *rit = Type(casted_ret);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> Sin(ArrayType const &x)
{
  ArrayType ret(x.shape());
  Sin(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = cos(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathNonFixedPointArray<ArrayType, void> Cos(ArrayType const &x,
                                                                   ArrayType &      ret)
{
  assert(ret.size() == x.size());
  kernels::Cos<typename ArrayType::Type> c;
  auto                                   x_it = x.cbegin();
  auto                                   rit  = ret.begin();
  while (x_it.is_valid())
  {
    c(*x_it, *rit);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> Cos(ArrayType const &x, ArrayType &ret)
{
  using Type = typename ArrayType::Type;
  assert(ret.size() == x.size());
  kernels::Cos<double> c;
  auto                 x_it = x.cbegin();
  auto                 rit  = ret.begin();
  double               casted_x;
  double               casted_ret;
  while (x_it.is_valid())
  {
    // TODO(800) - native fixed point implementation required - casting to double will not be
    // allowed
    casted_x   = double(*x_it);
    casted_ret = double(*rit);
    c(casted_x, casted_ret);
    *rit = Type(casted_ret);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> Cos(ArrayType const &x)
{
  ArrayType ret(x.shape());
  Cos(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = tan(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathNonFixedPointArray<ArrayType, void> Tan(ArrayType const &x,
                                                                   ArrayType &      ret)
{
  assert(ret.size() == x.size());
  kernels::Tan<typename ArrayType::Type> s;
  auto                                   x_it = x.cbegin();
  auto                                   rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> Tan(ArrayType const &x, ArrayType &ret)
{
  using Type = typename ArrayType::Type;
  assert(ret.size() == x.size());
  kernels::Tan<double> s;
  auto                 x_it = x.cbegin();
  auto                 rit  = ret.begin();
  double               casted_x;
  double               casted_ret;
  while (x_it.is_valid())
  {
    // TODO(800) - native fixed point implementation required - casting to double will not be
    // allowed
    casted_x   = double(*x_it);
    casted_ret = double(*rit);
    s(casted_x, casted_ret);
    *rit = Type(casted_ret);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> Tan(ArrayType const &x)
{
  ArrayType ret(x.shape());
  Tan(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = ASin(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathNonFixedPointArray<ArrayType, void> ASin(ArrayType const &x,
                                                                    ArrayType &      ret)
{
  assert(ret.size() == x.size());
  kernels::ASin<typename ArrayType::Type> s;
  auto                                    x_it = x.cbegin();
  auto                                    rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> ASin(ArrayType const &x, ArrayType &ret)
{
  using Type = typename ArrayType::Type;
  assert(ret.size() == x.size());
  kernels::ASin<double> s;
  auto                  x_it = x.cbegin();
  auto                  rit  = ret.begin();
  double                casted_x;
  double                casted_ret;
  while (x_it.is_valid())
  {
    // TODO(800) - native fixed point implementation required - casting to double will not be
    // allowed
    casted_x   = double(*x_it);
    casted_ret = double(*rit);
    s(casted_x, casted_ret);
    *rit = Type(casted_ret);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> ASin(ArrayType const &x)
{
  ArrayType ret(x.shape());
  ASin(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = ACos(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathNonFixedPointArray<ArrayType, void> ACos(ArrayType const &x,
                                                                    ArrayType &      ret)
{
  assert(ret.size() == x.size());
  kernels::ACos<typename ArrayType::Type> s;
  auto                                    x_it = x.cbegin();
  auto                                    rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> ACos(ArrayType const &x, ArrayType &ret)
{
  using Type = typename ArrayType::Type;
  assert(ret.size() == x.size());
  kernels::ACos<double> s;
  auto                  x_it = x.cbegin();
  auto                  rit  = ret.begin();
  double                casted_x;
  double                casted_ret;
  while (x_it.is_valid())
  {
    // TODO(800) - native fixed point implementation required - casting to double will not be
    // allowed
    casted_x   = double(*x_it);
    casted_ret = double(*rit);
    s(casted_x, casted_ret);
    *rit = Type(casted_ret);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> ACos(ArrayType const &x)
{
  ArrayType ret(x.shape());
  ACos(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = ATan(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathNonFixedPointArray<ArrayType, void> ATan(ArrayType const &x,
                                                                    ArrayType &      ret)
{
  assert(ret.size() == x.size());
  kernels::ATan<typename ArrayType::Type> s;
  auto                                    x_it = x.cbegin();
  auto                                    rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> ATan(ArrayType const &x, ArrayType &ret)
{
  using Type = typename ArrayType::Type;
  assert(ret.size() == x.size());
  kernels::ATan<double> s;
  auto                  x_it = x.cbegin();
  auto                  rit  = ret.begin();
  double                casted_x;
  double                casted_ret;
  while (x_it.is_valid())
  {
    // TODO(800) - native fixed point implementation required - casting to double will not be
    // allowed
    casted_x   = double(*x_it);
    casted_ret = double(*rit);
    s(casted_x, casted_ret);
    *rit = Type(casted_ret);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> ATan(ArrayType const &x)
{
  ArrayType ret(x.shape());
  ATan(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = SinH(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathNonFixedPointArray<ArrayType, void> SinH(ArrayType const &x,
                                                                    ArrayType &      ret)
{
  assert(ret.size() == x.size());
  kernels::SinH<typename ArrayType::Type> s;
  auto                                    x_it = x.cbegin();
  auto                                    rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> SinH(ArrayType const &x, ArrayType &ret)
{
  using Type = typename ArrayType::Type;
  assert(ret.size() == x.size());
  kernels::SinH<double> s;
  auto                  x_it = x.cbegin();
  auto                  rit  = ret.begin();
  double                casted_x;
  double                casted_ret;
  while (x_it.is_valid())
  {
    // TODO(800) - native fixed point implementation required - casting to double will not be
    // allowed
    casted_x   = double(*x_it);
    casted_ret = double(*rit);
    s(casted_x, casted_ret);
    *rit = Type(casted_ret);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> SinH(ArrayType const &x)
{
  ArrayType ret(x.shape());
  SinH(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = CosH(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathNonFixedPointArray<ArrayType, void> CosH(ArrayType const &x,
                                                                    ArrayType &      ret)
{
  assert(ret.size() == x.size());
  kernels::CosH<typename ArrayType::Type> s;
  auto                                    x_it = x.cbegin();
  auto                                    rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> CosH(ArrayType const &x, ArrayType &ret)
{
  using Type = typename ArrayType::Type;
  assert(ret.size() == x.size());
  kernels::CosH<double> s;
  auto                  x_it = x.cbegin();
  auto                  rit  = ret.begin();
  double                casted_x;
  double                casted_ret;
  while (x_it.is_valid())
  {
    // TODO(800) - native fixed point implementation required - casting to double will not be
    // allowed
    casted_x   = double(*x_it);
    casted_ret = double(*rit);
    s(casted_x, casted_ret);
    *rit = Type(casted_ret);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> CosH(ArrayType const &x)
{
  ArrayType ret(x.shape());
  CosH(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = TanH(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathNonFixedPointArray<ArrayType, void> TanH(ArrayType const &x,
                                                                    ArrayType &      ret)
{
  assert(ret.size() == x.size());
  kernels::TanH<typename ArrayType::Type> s;
  auto                                    x_it = x.cbegin();
  auto                                    rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> TanH(ArrayType const &x, ArrayType &ret)
{
  using Type = typename ArrayType::Type;
  assert(ret.size() == x.size());
  kernels::TanH<double> s;
  auto                  x_it = x.cbegin();
  auto                  rit  = ret.begin();
  double                casted_x;
  double                casted_ret;
  while (x_it.is_valid())
  {
    // TODO(800) - native fixed point implementation required - casting to double will not be
    // allowed
    casted_x   = double(*x_it);
    casted_ret = double(*rit);
    s(casted_x, casted_ret);
    *rit = Type(casted_ret);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> TanH(ArrayType const &x)
{
  ArrayType ret(x.shape());
  TanH(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = ASinH(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathNonFixedPointArray<ArrayType, void> ASinH(ArrayType const &x,
                                                                     ArrayType &      ret)
{
  assert(ret.size() == x.size());
  kernels::ASinH<typename ArrayType::Type> s;
  auto                                     x_it = x.cbegin();
  auto                                     rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> ASinH(ArrayType const &x,
                                                                  ArrayType &      ret)
{
  using Type = typename ArrayType::Type;
  assert(ret.size() == x.size());
  kernels::ASinH<double> s;
  auto                   x_it = x.cbegin();
  auto                   rit  = ret.begin();
  double                 casted_x;
  double                 casted_ret;
  while (x_it.is_valid())
  {
    // TODO(800) - native fixed point implementation required - casting to double will not be
    // allowed
    casted_x   = double(*x_it);
    casted_ret = double(*rit);
    s(casted_x, casted_ret);
    *rit = Type(casted_ret);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> ASinH(ArrayType const &x)
{
  ArrayType ret(x.shape());
  ASinH(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = ACosH(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathNonFixedPointArray<ArrayType, void> ACosH(ArrayType const &x,
                                                                     ArrayType &      ret)
{
  assert(ret.size() == x.size());
  kernels::ACosH<typename ArrayType::Type> s;
  auto                                     x_it = x.cbegin();
  auto                                     rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> ACosH(ArrayType const &x,
                                                                  ArrayType &      ret)
{
  using Type = typename ArrayType::Type;
  assert(ret.size() == x.size());
  kernels::ACosH<double> s;
  auto                   x_it = x.cbegin();
  auto                   rit  = ret.begin();
  double                 casted_x;
  double                 casted_ret;
  while (x_it.is_valid())
  {
    // TODO(800) - native fixed point implementation required - casting to double will not be
    // allowed
    casted_x   = double(*x_it);
    casted_ret = double(*rit);
    s(casted_x, casted_ret);
    *rit = Type(casted_ret);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> ACosH(ArrayType const &x)
{
  ArrayType ret(x.shape());
  ACosH(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = ATanH(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathNonFixedPointArray<ArrayType, void> ATanH(ArrayType const &x,
                                                                     ArrayType &      ret)
{
  assert(ret.size() == x.size());
  kernels::ATanH<typename ArrayType::Type> s;
  auto                                     x_it = x.cbegin();
  auto                                     rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> ATanH(ArrayType const &x,
                                                                  ArrayType &      ret)
{
  using Type = typename ArrayType::Type;
  assert(ret.size() == x.size());
  kernels::ATanH<double> s;
  auto                   x_it = x.cbegin();
  auto                   rit  = ret.begin();
  double                 casted_x;
  double                 casted_ret;
  while (x_it.is_valid())
  {
    // TODO(800) - native fixed point implementation required - casting to double will not be
    // allowed
    casted_x   = double(*x_it);
    casted_ret = double(*rit);
    s(casted_x, casted_ret);
    *rit = Type(casted_ret);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> ATanH(ArrayType const &x)
{
  ArrayType ret(x.shape());
  ATanH(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = Sin(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> Sin(Type const &x, Type &ret)
{
  kernels::Sin<Type> s;
  s(x, ret);
}
template <typename Type>
fetch::math::meta::IfIsFixedPoint<Type, void> Sin(Type const &x, Type &ret)
{
  double               casted_ret = double(ret);
  double               dx{x};
  kernels::Sin<double> s;
  s(dx, casted_ret);
  ret = Type(casted_ret);
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> Sin(Type const &x)
{
  Type ret;
  Sin(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = cos(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> Cos(Type const &x, Type &ret)
{
  kernels::Cos<Type> c;
  c(x, ret);
}
template <typename Type>
fetch::math::meta::IfIsFixedPoint<Type, void> Cos(Type const &x, Type &ret)
{
  double               casted_ret = double(ret);
  double               dx{x};
  kernels::Cos<double> c;
  c(dx, casted_ret);
  ret = Type(casted_ret);
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> Cos(Type const &x)
{
  Type ret;
  Cos(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = tan(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> Tan(Type const &x, Type &ret)
{
  kernels::Tan<Type> t;
  t(x, ret);
}
template <typename Type>
fetch::math::meta::IfIsFixedPoint<Type, void> Tan(Type const &x, Type &ret)
{
  double               casted_ret = double(ret);
  double               dx{x};
  kernels::Tan<double> t;
  t(dx, casted_ret);
  ret = Type(casted_ret);
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> Tan(Type const &x)
{
  Type ret;
  Tan(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = ASin(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> ASin(Type const &x, Type &ret)
{
  kernels::ASin<Type> t;
  t(x, ret);
}
template <typename Type>
fetch::math::meta::IfIsFixedPoint<Type, void> ASin(Type const &x, Type &ret)
{
  double                casted_ret = double(ret);
  double                dx{x};
  kernels::ASin<double> t;
  t(dx, casted_ret);
  ret = Type(casted_ret);
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> ASin(Type const &x)
{
  Type ret;
  ASin(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = ACos(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> ACos(Type const &x, Type &ret)
{
  kernels::ACos<Type> t;
  t(x, ret);
}
template <typename Type>
fetch::math::meta::IfIsFixedPoint<Type, void> ACos(Type const &x, Type &ret)
{
  double                casted_ret = double(ret);
  double                dx{x};
  kernels::ACos<double> t;
  t(dx, casted_ret);
  ret = Type(casted_ret);
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> ACos(Type const &x)
{
  Type ret;
  ACos(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = ATan(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> ATan(Type const &x, Type &ret)
{
  kernels::ATan<Type> t;
  t(x, ret);
}
template <typename Type>
fetch::math::meta::IfIsFixedPoint<Type, void> ATan(Type const &x, Type &ret)
{
  double                casted_ret = double(ret);
  double                dx{x};
  kernels::ATan<double> t;
  t(dx, casted_ret);
  ret = Type(casted_ret);
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> ATan(Type const &x)
{
  Type ret;
  ATan(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = SinH(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> SinH(Type const &x, Type &ret)
{
  kernels::SinH<Type> t;
  t(x, ret);
}
template <typename Type>
fetch::math::meta::IfIsFixedPoint<Type, void> SinH(Type const &x, Type &ret)
{
  double                casted_ret = double(ret);
  double                dx{x};
  kernels::SinH<double> t;
  t(dx, casted_ret);
  ret = Type(casted_ret);
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> SinH(Type const &x)
{
  Type ret;
  SinH(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = CosH(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> CosH(Type const &x, Type &ret)
{
  kernels::CosH<Type> t;
  t(x, ret);
}
template <typename Type>
fetch::math::meta::IfIsFixedPoint<Type, void> CosH(Type const &x, Type &ret)
{
  double                casted_ret = double(ret);
  double                dx{x};
  kernels::CosH<double> t;
  t(dx, casted_ret);
  ret = Type(casted_ret);
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> CosH(Type const &x)
{
  Type ret;
  CosH(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = TanH(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> TanH(Type const &x, Type &ret)
{
  kernels::TanH<Type> t;
  t(x, ret);
}
template <typename Type>
fetch::math::meta::IfIsFixedPoint<Type, void> TanH(Type const &x, Type &ret)
{
  double                casted_ret = double(ret);
  double                dx{x};
  kernels::TanH<double> t;
  t(dx, casted_ret);
  ret = Type(casted_ret);
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> TanH(Type const &x)
{
  Type ret;
  TanH(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = ASinH(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> ASinH(Type const &x, Type &ret)
{
  kernels::ASinH<Type> t;
  t(x, ret);
}
template <typename Type>
fetch::math::meta::IfIsFixedPoint<Type, void> ASinH(Type const &x, Type &ret)
{
  double                 casted_ret = double(ret);
  double                 dx{x};
  kernels::ASinH<double> t;
  t(dx, casted_ret);
  ret = Type(casted_ret);
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> ASinH(Type const &x)
{
  Type ret;
  ASinH(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = ACosH(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> ACosH(Type const &x, Type &ret)
{
  kernels::ACosH<Type> t;
  t(x, ret);
}
template <typename Type>
fetch::math::meta::IfIsFixedPoint<Type, void> ACosH(Type const &x, Type &ret)
{
  double                 casted_ret = double(ret);
  double                 dx{x};
  kernels::ACosH<double> t;
  t(dx, casted_ret);
  ret = Type(casted_ret);
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> ACosH(Type const &x)
{
  Type ret;
  ACosH(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = ATanH(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsNonFixedPointArithmetic<Type, void> ATanH(Type const &x, Type &ret)
{
  kernels::ATanH<Type> t;
  t(x, ret);
}
template <typename Type>
fetch::math::meta::IfIsFixedPoint<Type, void> ATanH(Type const &x, Type &ret)
{
  double                 casted_ret = double(ret);
  double                 dx{x};
  kernels::ATanH<double> t;
  t(dx, casted_ret);
  ret = Type(casted_ret);
}
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> ATanH(Type const &x)
{
  Type ret;
  ATanH(x, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch