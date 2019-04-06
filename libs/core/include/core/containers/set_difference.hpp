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

#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <unordered_set>

namespace fetch {

template <typename K>
std::unordered_set<K> operator-(std::unordered_set<K> const &lhs, std::unordered_set<K> const &rhs)
{
  std::unordered_set<K> result;

  std::copy_if(lhs.begin(), lhs.end(), std::inserter(result, result.begin()),
               [&rhs](K const &item) { return rhs.find(item) == rhs.end(); });

  return result;
}

}  // namespace fetch
