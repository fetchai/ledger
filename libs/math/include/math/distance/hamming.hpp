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
#include "math/base_types.hpp"

namespace fetch {
namespace math {
namespace distance {

template <typename ArrayType>
typename ArrayType::Type Hamming(ArrayType const &a, ArrayType const &b)
{
  detailed_assert(a.size() == b.size());
  using Type     = typename ArrayType::Type;
  using SizeType = fetch::math::SizeType;

  auto result = Type(0);
  auto count  = SizeType(0);
  for (auto &val : a)
  {
    // TODO(private issue 193): implement boolean only array
    if (val != b.At(count))
    {
      result += Type(1);
    }
    count++;
  }

  return result;
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
