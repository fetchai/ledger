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

#include "core/assert.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename ArrayType>
typename ArrayType::Type Braycurtis(ArrayType const &a, ArrayType const &b)
{
  detailed_assert(a.size() == b.size());
  using DataType = typename ArrayType::Type;

  DataType numerator   = Sum(Subtract(a, b));
  DataType denominator = Sum(Add(a, b));

  return numerator / denominator;
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
