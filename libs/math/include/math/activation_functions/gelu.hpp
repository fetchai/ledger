#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "math/standard_functions/pow.hpp"
#include "math/trigonometry.hpp"
#include "vectorise/math/max.hpp"

#include <cassert>

namespace fetch {
namespace math {

/**
 * Guassian error linear unit (approximated)
 * 0.5x(1+tanh(0.797885x+0.035677x^3))
 * @tparam ArrayType
 * @param t
 * @param ret
 */
template <typename ArrayType>
void Gelu(ArrayType const &t, ArrayType &ret)
{
  assert(t.size() == ret.size());
  using DataType = typename ArrayType::Type;

  DataType  one{1};
  DataType  half = Type<DataType>("0.5");
  DataType  three{3};
  DataType  coeff1 = Type<DataType>("0.797885");
  DataType  coeff2 = Type<DataType>("0.035677");
  ArrayType intermediate(t.shape());

  Multiply(t, coeff1, intermediate);
  Pow(t, three, ret);
  Multiply(ret, coeff2, ret);
  Add(intermediate, ret, ret);
  TanH(ret, ret);
  Add(ret, one, ret);
  Multiply(t, ret, ret);
  Multiply(ret, half, ret);
}

template <typename ArrayType>
ArrayType Gelu(ArrayType const &t)
{
  ArrayType ret(t.shape());
  Gelu(t, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
