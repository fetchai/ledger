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

#include "math/comparison.hpp"
#include "math/meta/math_type_traits.hpp"

/**
 * Limit array's minimum and maximum value
 * @param x
 */

namespace fetch {
namespace math {

///////////////////////
/// IMPLEMENTATIONS ///
///////////////////////

template <typename Type>
void Clamp(Type const &x, Type const &min, Type const &max, Type &ret)
{
  assert(min <= max);

  if (x <= min)
  {
    ret = min;
    return;
  }
  if (x >= max)
  {
    ret = max;
    return;
  }
  ret = x;
}

template <typename Type>
void Clamp(Type const &min, Type const &max, Type &ret)
{
  assert(min <= max);

  if (ret <= min)
  {
    ret = min;
    return;
  }
  if (ret >= max)
  {
    ret = max;
  }
}

//////////////////
/// INTERFACES ///
//////////////////

template <typename Type>
meta::IfIsArithmetic<Type, Type> Clamp(Type const &x, Type const &min, Type const &max)
{
  Type ret;
  Clamp(x, min, max, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Clamp(ArrayType const &               array,
                                           typename ArrayType::Type const &min,
                                           typename ArrayType::Type const &max, ArrayType &ret)
{
  assert(ret.shape() == array.shape());
  auto ret_it = ret.begin();
  auto in_it  = array.begin();
  while (in_it.is_valid())
  {
    Clamp(*in_it, min, max, *ret_it);
    ++ret_it;
    ++in_it;
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Clamp(ArrayType const &               array,
                                                typename ArrayType::Type const &min,
                                                typename ArrayType::Type const &max)
{
  ArrayType ret{array.shape()};
  Clamp(array, min, max, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Clamp(typename ArrayType::Type const &min,
                                           typename ArrayType::Type const &max, ArrayType &ret)
{
  auto ret_it = ret.begin();
  while (ret_it.is_valid())
  {
    Clamp(min, max, *ret_it);
    ++ret_it;
  }
}

}  // namespace math
}  // namespace fetch