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
#include "math/fundamental_operators.hpp"
#include "math/standard_functions/sqrt.hpp"
#include "math/matrix_operations.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace correlation {

template <typename ArrayType>
void Cosine(ArrayType const &a, ArrayType const &b, typename ArrayType::Type &r)
{
  assert(a.size() == b.size());

  // get inner product
  ArrayType dp_ret = fetch::math::DotTranspose(a, b);
  assert(dp_ret.size() == 1);
  r = dp_ret[0];

  // self products
  typename ArrayType::Type a_r, b_r, denom_r;

  dp_ret = fetch::math::DotTranspose(a, a);
  assert(dp_ret.size() == 1);
  a_r = dp_ret[0];

  dp_ret = fetch::math::DotTranspose(b, b);
  assert(dp_ret.size() == 1);
  b_r = dp_ret[0];

  denom_r = Sqrt(fetch::math::Multiply(a_r, b_r));

  r /= denom_r;
}

template <typename ArrayType>
typename ArrayType::Type Cosine(ArrayType const &a, ArrayType const &b)
{
  typename ArrayType::Type ret;
  Cosine(a, b, ret);
  return ret;
}

}  // namespace correlation
}  // namespace math
}  // namespace fetch
