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
#include "math/comparison.hpp"
#include "math/fundamental_operators.hpp"
#include <cmath>

namespace fetch {
namespace math {
namespace correlation {

template <typename ArrayType>
inline typename ArrayType::Type Jaccard(ArrayType const &a, ArrayType const &b)
{
  assert(a.size() == b.size());
  using DataType = typename ArrayType::Type;
  using SizeType = typename ArrayType::SizeType;

  DataType sumA  = 0;
  DataType sumB  = 0;
  SizeType count = 0;
  for (auto &val : a)
  {
    sumA += Min(val != 0, b.At(count) != 0);
    sumB += Max(val != 0, b.At(count) != 0);
    count++;
  }

  return Divide(sumA, sumB);
}

template <typename ArrayType>
inline typename ArrayType::Type GeneralisedJaccard(ArrayType const &a, ArrayType const &b)
{
  assert(a.size() == b.size());
  using DataType = typename ArrayType::Type;
  using SizeType = typename ArrayType::SizeType;

  DataType sumA  = 0;
  DataType sumB  = 0;
  SizeType count = 0;
  for (auto &val : a)
  {
    sumA += Min(val, b.At(count));
    sumB += Max(val, b.At(count));
    count++;
  }

  return Divide(sumA, sumB);
}

}  // namespace correlation
}  // namespace math
}  // namespace fetch
