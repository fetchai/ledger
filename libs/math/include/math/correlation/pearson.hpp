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
#include "math/standard_functions/pow.hpp"
#include "math/standard_functions/sqrt.hpp"
#include "math/statistics/mean.hpp"
#include <cmath>

namespace fetch {
namespace math {
namespace correlation {

template <typename ArrayType>
inline typename ArrayType::Type Pearson(ArrayType const &a, ArrayType const &b)
{
  assert(a.size() == b.size());
  using Type     = typename ArrayType::Type;
  using SizeType = typename ArrayType::SizeType;

  Type meanA  = fetch::math::statistics::Mean(a);
  Type meanB  = fetch::math::statistics::Mean(b);
  Type innerA = Sum(Square(Subtract(a, meanA)));
  Type innerB = Sum(Square(Subtract(b, meanB)));

  Type     numerator = 0;
  SizeType count     = 0;
  for (auto &val : a)
  {
    numerator += ((val - meanA) * (b.At(count) - meanB));
    ++count;
  }

  Type denom = Type(fetch::math::Multiply(fetch::math::Sqrt(innerA), fetch::math::Sqrt(innerB)));

  return Type(numerator / denom);
}

}  // namespace correlation
}  // namespace math
}  // namespace fetch
