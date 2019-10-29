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

#include "math/fundamental_operators.hpp"
#include "math/standard_functions/exp.hpp"

namespace fetch {
namespace math {

/**
 * The sigmoid function - numerically stable
 * @tparam ArrayType
 * @param t
 * @param ret
 */
template <typename ArrayType>
void Sigmoid(ArrayType const &t, ArrayType &ret)
{
  using Type = typename ArrayType::Type;

  auto array_it = t.cbegin();
  auto rit      = ret.begin();
  Type zero{0};
  Type one{1};
  Type min_one{-1};

  while (array_it.is_valid())
  {
    if (*array_it >= zero)
    {
      Multiply(min_one, *array_it, *rit);
      Exp(*rit, *rit);
      Add(*rit, one, *rit);
      Divide(one, *rit, *rit);
    }
    else
    {
      Exp(*array_it, *rit);
      Divide(*rit, *rit + one, *rit);
    }
    ++array_it;
    ++rit;
  }
}

template <typename ArrayType>
ArrayType Sigmoid(ArrayType const &t)
{
  ArrayType ret(t.shape());
  Sigmoid(t, ret);

  return ret;
}

}  // namespace math
}  // namespace fetch
