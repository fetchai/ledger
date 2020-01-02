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
#include "vectorise/math/max.hpp"

#include <cassert>

namespace fetch {
namespace math {

/**
 * Rectified linear unit
 * @tparam ArrayType
 * @param t
 * @param ret
 */
template <typename ArrayType>
void Relu(ArrayType const &t, ArrayType &ret)
{
  assert(t.size() == ret.size());

  auto it  = t.cbegin();
  auto rit = ret.begin();
  while (it.is_valid())
  {
    *rit = fetch::vectorise::Max(*it, typename ArrayType::Type(0));
    ++it;
    ++rit;
  }
}

template <typename ArrayType>
ArrayType Relu(ArrayType const &t)
{
  ArrayType ret(t.shape());
  Relu(t, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
