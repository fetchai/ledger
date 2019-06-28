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
#include "math/standard_functions/pow.hpp"
#include "math/comparison.hpp"
#include "math/meta/math_type_traits.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename ArrayType>
inline meta::IfIsMathArray<ArrayType, typename ArrayType::Type> Minkowski(ArrayType const &a, ArrayType const &b,
                                          typename ArrayType::Type n)
{
  detailed_assert(a.size() == b.size());
  using DataType = typename ArrayType::Type;
  using SizeType = typename ArrayType::SizeType;

  SizeType count = 0;
  DataType sum   = 0;
  for (auto &val : a)
  {
    sum += Pow(Max(val, b.At(count)) - Min(val, b.At(count)), n);
  }
  return Pow(sum, 1. / double(n));
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
