#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iterator>

namespace fetch {

template <typename K>
std::unordered_set<K> operator-(std::unordered_set<K> const &lhs, std::unordered_set<K> const &rhs)
{
  std::unordered_set<K> result;

  std::copy_if(
    lhs.begin(),
    lhs.end(),
    std::inserter(result, result.begin()),
    [&rhs] (K const &item)
    {
      return rhs.find(item) == rhs.end();
    }
  );

  return result;
}

//template <typename K, typename V>
//std::unordered_map<K, V> operator-(std::unordered_map<K, V> const &lhs, std::unordered_set<K> const &rhs)
//{
//  std::unordered_map<K, V> result;
//
//  std::copy_if(
//    lhs.begin(),
//    lhs.end(),
//    std::inserter(result),
//    [&rhs] (auto const &item)
//    {
//      return rhs.find(item.first) == rhs.end();
//    }
//  );
//
//  return result;
//}

} // namespace fetch
