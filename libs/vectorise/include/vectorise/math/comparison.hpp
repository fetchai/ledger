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

#include "vectorise/meta/math_type_traits.hpp"

#include <cassert>

namespace fetch {
namespace math {

/**
 * Min function for two values
 * @tparam T
 * @param datum1
 * @param datum2
 * @return
 */
template <typename T>
inline void Min(T const &a, T const &b, T &ret)
{
  ret = std::min(a, b);
}
template <typename T>
inline T Min(T const &a, T const &b)
{
  T ret = std::min(a, b);
  return ret;
}

}  // namespace math
}  // namespace fetch
