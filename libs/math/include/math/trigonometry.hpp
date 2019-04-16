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
#include "math/meta/math_type_traits.hpp"
#include <cassert>
#include <cmath>

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

  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    *rit = std::sin(*x_it);
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
  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    *rit = std::cos(*x_it);
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
  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    *rit = std::tan(*x_it);
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
  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    *rit = std::asin(*x_it);
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
  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    *rit = std::acos(*x_it);
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
  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    *rit = std::atan(*x_it);
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
 * The atan2() function in C++ returns the inverse tangent of a coordinate in radians.
 * @param y: this value represents the proportion of the y-coordinate.
 * @param x: this value represents the proportion of the x-coordinate
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> ATan2(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    *rit = std::atan2(*x_it);
    ++x_it;
    ++rit;
  }
}

template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> ATan2(ArrayType const &x)
{

  ArrayType ret(x.shape());
  ATan2(x, ret);
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
  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    *rit = std::sinh(*x_it);
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
 * maps every element of the array x to ret = ATan(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> CosH(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    *rit = std::cosh(*x_it);
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
 * maps every element of the array x to ret = ATan(x)
 * @param x - array
 */
template <typename ArrayType>
fetch::math::meta::IfIsMathNonFixedPointArray<ArrayType, void> TanH(ArrayType const &x,
                                                                    ArrayType &      ret)
{
  ASSERT(ret.size() == x.size());
  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    *rit = std::tanh(*x_it);
    ++x_it;
    ++rit;
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> TanH(ArrayType const &x, ArrayType &ret)
{
  ASSERT(ret.size() == x.size());
  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    // TODO(800) - native fixed point implementation required - casting to double will not be
    *rit = typename ArrayType::Type(std::tanh(double(*x_it)));
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
  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    *rit = std::asinh(*x_it);
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
  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    *rit = std::acosh(*x_it);
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
  auto x_it = x.cbegin();
  auto rit  = ret.begin();
  while (x_it.is_valid())
  {
    *rit = std::atanh(*x_it);
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

}  // namespace math
}  // namespace fetch