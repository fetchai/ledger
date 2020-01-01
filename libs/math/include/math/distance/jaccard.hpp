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
#include "math/correlation/jaccard.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename ArrayType>
typename ArrayType::Type Jaccard(ArrayType const &a, ArrayType const &b)
{
  using Type = typename ArrayType::Type;
  return Type(1) - correlation::Jaccard(a, b);
}

template <typename ArrayType>
typename ArrayType::Type GeneralisedJaccard(ArrayType const &a, ArrayType const &b)
{
  using Type = typename ArrayType::Type;
  return Type(1) - correlation::GeneralisedJaccard(a, b);
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
