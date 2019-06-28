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
#include "math/matrix_operations.hpp"
#include "math/standard_functions/sqrt.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace correlation {

template <typename ArrayType>
void Cosine(ArrayType const &a, ArrayType const &b, typename ArrayType::Type &r)
{
  assert(a.size() == b.size());
  // self products
  typename ArrayType::Type a_r, b_r, denom_r;
  ArrayType                dp_ret;

  // get inner product
  if ((b.shape()[0] == 1) && (a.shape()[0] == 1))
  {
    dp_ret = fetch::math::DotTranspose(a, b);
  }
  else
  {
    dp_ret = fetch::math::TransposeDot(a, b);
  }
  assert(dp_ret.size() == 1);
  r = dp_ret[0];

  if (b.shape()[0] == 1)
  {
    dp_ret = fetch::math::DotTranspose(a, a);
  }
  else
  {
    dp_ret = fetch::math::TransposeDot(a, a);
  }
  assert(dp_ret.size() == 1);
  a_r = dp_ret[0];
  fetch::math::Sqrt(a_r, a_r);

  if (b.shape()[0] == 1)
  {
    dp_ret = fetch::math::DotTranspose(b, b);
  }
  else
  {
    dp_ret = fetch::math::TransposeDot(b, b);
  }
  assert(dp_ret.size() == 1);
  b_r = dp_ret[0];
  fetch::math::Sqrt(b_r, b_r);

  fetch::math::Multiply(a_r, b_r, denom_r);

  fetch::math::Divide(r, denom_r, r);

  assert(r <= 1.0);
  assert(r >= -1.0);
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
