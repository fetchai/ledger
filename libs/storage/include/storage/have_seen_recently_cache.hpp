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

#include <deque>
#include <unordered_set>

namespace fetch {
namespace storage {

/**
 * Not thread safe.
 *
 * This class provides the functionality that you can add elements to it,
 * they will be held in the order they were added up to a fixed size,
 * and you can query whether they are still there (have been recently seen).
 *
 * Due to the implementation behaviour, adding the same item when it is
 * already Seen may result in it returning it as not been seen sooner than expected.
 *
 */
template <typename T>
class HaveSeenRecentlyCache
{
public:
  explicit HaveSeenRecentlyCache(uint64_t size)
    : size_{size}
  {}

  bool Seen(T const &item)
  {
    return seen_set_.find(item) != seen_set_.end();
  }

  void Add(T const &item)
  {
    seen_set_.insert(item);
    seen_deque_.push_front(item);

    while (seen_deque_.size() > size_)
    {
      seen_set_.erase(seen_deque_.back());
      seen_deque_.pop_back();
    }
  }

private:
  // To achieve the functionality the class maintains both a set and a deque
  // of the elements added
  using SeenSet   = std::unordered_set<T>;
  using SeenDeque = std::deque<T>;

  uint64_t  size_ = 0;
  SeenSet   seen_set_;
  SeenDeque seen_deque_;
};

}  // namespace storage
}  // namespace fetch
