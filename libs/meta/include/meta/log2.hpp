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

#include "meta/type_traits.hpp"

namespace fetch {
namespace meta {

template <typename T>
constexpr EnableIf<IsInteger<T>, bool> IsLog2(T val) noexcept
{
  return static_cast<bool>(val && !(val & (val - 1)));
}

template <typename T>
constexpr EnableIf<IsInteger<T>, T> Log2(T val) noexcept
{
  return ((val > 1) ? (1 + Log2(val >> 1)) : 0);
}

}  // namespace meta
}  // namespace fetch
