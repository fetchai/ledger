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
#include "math/matrix_operations.hpp"
#include "math/standard_functions/pow.hpp"

namespace fetch {
namespace math {

template <typename ArrayType>
meta::IfIsMathFixedPointArray<ArrayType, typename ArrayType::Type> L2Loss(ArrayType const &A)
{
  using Type = typename ArrayType::Type;
  auto tmp   = Sum(Square(A));
  tmp /= Type{2};
  return tmp;
}

template <typename ArrayType>
meta::IfIsMathNonFixedPointArray<ArrayType, typename ArrayType::Type> L2Loss(ArrayType const &at)
{
  auto a = at.data();

  using Type = typename ArrayType::Type;

  Type l2loss = a.in_parallel().SumReduce([](auto const &x) { return x * x; });
  l2loss /= Type{2};
  return l2loss;
}

}  // namespace math
}  // namespace fetch
