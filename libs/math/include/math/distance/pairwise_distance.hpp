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
#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename ArrayType, typename F>
inline ArrayType &PairWiseDistance(ArrayType &r, ArrayType const &a, F &&metric)
{
  using SizeType = typename ArrayType::SizeType;

  detailed_assert(r.height() == 1);
  detailed_assert(r.width() == (a.height() * (a.height() - 1) / 2));

  SizeType offset1 = 0;
  SizeType k       = 0;

  for (SizeType i = 0; i < a.height(); ++i)
  {
    ArrayType slice1 = a.data().slice(offset1, a.width());
    offset1 += a.padded_width();
    SizeType offset2 = offset1;

    for (SizeType j = i + 1; j < a.height(); ++j)
    {
      ArrayType slice2 = a.data().slice(offset2, a.width());
      offset2 += a.padded_width();
      r(SizeType(0), k++) = metric(slice1, slice2);
    }
  }

  return r;
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
