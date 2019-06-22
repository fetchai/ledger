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
inline void Clamp(Type const &min, Type const &max, Type &ret)
{
  assert(min <= max);

  if (ret <= min)
  {
    ret = min;
  }
  else if (ret >= max)
  {
    ret = max;
  }
}

//////////////////
/// INTERFACES ///
//////////////////

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

template <typename Type>
meta::IfIsArithmetic<Type, void> Clamp(Type const &min, Type const &max, Type &ret)
{
  Clamp(min, max, ret);
}

}  // namespace math
}  // namespace fetch
