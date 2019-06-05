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
#include "math/fundamental_operators.hpp"
#include "math/standard_functions/exp.hpp"

#include <cassert>

namespace fetch {
namespace math {

/**
 * Exponential linear unit
 * @tparam ArrayType
 * @param t
 * @param a
 * @param ret
 */
template <typename ArrayType>
void Elu(ArrayType const &t, typename ArrayType::Type &a, ArrayType &ret)
{
  assert(t.size() == ret.size());
  using DataType = typename ArrayType::Type;

  DataType zero{0};
  DataType one{1};

  auto it  = t.cbegin();
  auto rit = ret.begin();
  while (it.is_valid())
  {
    // f(x)=x for x>=0
    // f(x)=a*(e^x-1) for x<0
    if (*it < zero)
    {
      Exp(*it, *rit);
      Subtract(*rit, one, *rit);
      Multiply(a, *rit, *rit);
    }
    else
    {
      *rit = *it;
    }
    ++it;
    ++rit;
  }
}

template <typename ArrayType>
ArrayType Elu(ArrayType const &t, typename ArrayType::Type &a)
{
  ArrayType ret(t.shape());
  Elu(t, a, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
