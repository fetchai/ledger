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
#include "math/meta/math_type_traits.hpp"
#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename ArrayType, typename F>
inline meta::IfIsMathArray<ArrayType, ArrayType> &PairWiseDistance(ArrayType const &a, F &&metric, ArrayType &r)
{
  using SizeType = typename ArrayType::SizeType;

  detailed_assert(r.shape(0) == 1);
  detailed_assert(r.shape(1) == (a.shape(0) * (a.shape(0) - 1) / 2));

  SizeType k = 0;

  for (SizeType i = 0; i < a.shape(0); ++i)
  {
    for (SizeType j = i + 1; j < a.shape(0); ++j)
    {
      ArrayType slice1 = a.Slice(i).Copy();
      ArrayType slice2 = a.Slice(j).Copy();
      r(SizeType{0}, k++) = metric(slice1, slice2);
    }
  }

  return r;
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
