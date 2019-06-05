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

#include "math/kernels/trigonometry.hpp"
#include "math/meta/math_type_traits.hpp"

#include <cassert>

namespace fetch {
namespace math {

/**
 * maps every element of the array x to ret = sin(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> Sin(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  kernels::Sin s;
  auto         x_it = x.cbegin();
  auto         rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
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
fetch::math::meta::IfIsMathArray<ArrayType, void> Cos(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  kernels::Cos c;
  auto         x_it = x.cbegin();
  auto         rit  = ret.begin();
  while (x_it.is_valid())
  {
    c(*x_it, *rit);
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
fetch::math::meta::IfIsMathArray<ArrayType, void> Tan(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  kernels::Tan s;
  auto         x_it = x.cbegin();
  auto         rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
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
fetch::math::meta::IfIsMathArray<ArrayType, void> ASin(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  kernels::ASin s;
  auto          x_it = x.cbegin();
  auto          rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
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
fetch::math::meta::IfIsMathArray<ArrayType, void> ACos(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  kernels::ACos s;
  auto          x_it = x.cbegin();
  auto          rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
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
fetch::math::meta::IfIsMathArray<ArrayType, void> ATan(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  kernels::ATan s;
  auto          x_it = x.cbegin();
  auto          rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
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
fetch::math::meta::IfIsMathArray<ArrayType, void> SinH(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  kernels::SinH s;
  auto          x_it = x.cbegin();
  auto          rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
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
fetch::math::meta::IfIsMathArray<ArrayType, void> CosH(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  kernels::CosH s;
  auto          x_it = x.cbegin();
  auto          rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
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
fetch::math::meta::IfIsMathArray<ArrayType, void> TanH(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  kernels::TanH s;
  auto          x_it = x.cbegin();
  auto          rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
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
fetch::math::meta::IfIsMathArray<ArrayType, void> ASinH(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  kernels::ASinH s;
  auto           x_it = x.cbegin();
  auto           rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
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
fetch::math::meta::IfIsMathArray<ArrayType, void> ACosH(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  kernels::ACosH s;
  auto           x_it = x.cbegin();
  auto           rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
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
fetch::math::meta::IfIsMathArray<ArrayType, void> ATanH(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  kernels::ATanH s;
  auto           x_it = x.cbegin();
  auto           rit  = ret.begin();
  while (x_it.is_valid())
  {
    s(*x_it, *rit);
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
fetch::math::meta::IfIsArithmetic<Type, void> Sin(Type const &x, Type &ret)
{
  kernels::Sin s;
  s(x, ret);
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
fetch::math::meta::IfIsArithmetic<Type, void> Cos(Type const &x, Type &ret)
{
  kernels::Cos c;
  c(x, ret);
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
fetch::math::meta::IfIsArithmetic<Type, void> Tan(Type const &x, Type &ret)
{
  kernels::Tan t;
  t(x, ret);
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
fetch::math::meta::IfIsArithmetic<Type, void> ASin(Type const &x, Type &ret)
{
  kernels::ASin t;
  t(x, ret);
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
fetch::math::meta::IfIsArithmetic<Type, void> ACos(Type const &x, Type &ret)
{
  kernels::ACos t;
  t(x, ret);
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
fetch::math::meta::IfIsArithmetic<Type, void> ATan(Type const &x, Type &ret)
{
  kernels::ATan t;
  t(x, ret);
}

template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> ATan(Type const &x)
{
  Type ret;
  ATan(x, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = ATan(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, void> ATan2(Type const &x, Type const &y, Type &ret)
{
  kernels::ATan2 t;
  t(x, y, ret);
}

template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, Type> ATan2(Type const &x, Type const &y)
{
  Type ret;
  ATan2(x, y, ret);
  return ret;
}

/**
 * maps every element of the array x to ret = SinH(x)
 * @param x - array
 */
template <typename Type>
fetch::math::meta::IfIsArithmetic<Type, void> SinH(Type const &x, Type &ret)
{
  kernels::SinH t;
  t(x, ret);
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
fetch::math::meta::IfIsArithmetic<Type, void> CosH(Type const &x, Type &ret)
{
  kernels::CosH t;
  t(x, ret);
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
fetch::math::meta::IfIsArithmetic<Type, void> TanH(Type const &x, Type &ret)
{
  kernels::TanH t;
  t(x, ret);
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
fetch::math::meta::IfIsArithmetic<Type, void> ASinH(Type const &x, Type &ret)
{
  kernels::ASinH t;
  t(x, ret);
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
fetch::math::meta::IfIsArithmetic<Type, void> ACosH(Type const &x, Type &ret)
{
  kernels::ACosH t;
  t(x, ret);
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
fetch::math::meta::IfIsArithmetic<Type, void> ATanH(Type const &x, Type &ret)
{
  kernels::ATanH t;
  t(x, ret);
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
