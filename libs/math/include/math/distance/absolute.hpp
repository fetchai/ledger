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

#include <cassert>

namespace fetch {
namespace math {
namespace distance {

template <typename ArrayType>
typename ArrayType::Type AbsoluteDistance(ArrayType const &A, ArrayType const &B)
{
  using Type = typename ArrayType::Type;
  auto it1   = A.cbegin();
  auto it2   = B.cbegin();
  assert(it1.size() == it2.size());
  auto ret = Type{0};
  Type d;

  while (it1.is_valid())
  {
    if ((*it1 > Type{0}) && (*it2 > Type{0}))
    {
      d = (*it1) - (*it2);
      if (d < Type{0})
      {
        d = -d;
      }
    }
    else if ((*it1) > Type{0})
    {
      d = (*it1) - (*it2);
    }
    else if ((*it2 > Type{0}))
    {
      d = (*it2) - (*it1);
    }
    else
    {
      d = (*it2) - (*it1);
      if (d < Type{0})
      {
        d = -d;
      }
    }

    ret += d;
    ++it1;
    ++it2;
  }

  return ret;
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
