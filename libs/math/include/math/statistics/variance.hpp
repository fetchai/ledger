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
#include "math/statistics/mean.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace statistics {

template <typename A>
typename A::Type Variance(A const &a)
{
  using DataType = typename A::Type;

  DataType m = Mean(a);
  DataType v{0};
  for (auto &val : a)
  {
    v += ((val - m) * (val - m));
  }
  v /= static_cast<DataType>(a.size());

  return v;
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch
