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

#include <unordered_set>

namespace fetch {

/**
 * Joins the set of values from one set to another
 *
 * @tparam K The key type
 * @tparam H The hash type
 * @tparam E The equality type
 * @param input The input set
 * @param other The other set to joined to the first
 * @return The new set containing the joined values
 */
template <typename K, typename H, typename E>
std::unordered_set<K, H, E> operator+(std::unordered_set<K, H, E>        input,
                                      std::unordered_set<K, H, E> const &other)
{
  for (auto const &address : other)
  {
    input.emplace(address);
  }

  return input;
}

}  // namespace fetch
