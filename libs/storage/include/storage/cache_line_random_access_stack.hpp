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
#include "storage/random_access_stack.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <string>
#include <unordered_map>

namespace fetch {
namespace storage {

/**
 * The CacheLineRandomAccessStack owns a stack of type T (RandomAccessStack), and provides
 * caching in an invisible manner.
 *
 * It does this by maintaining a quick access structure (data_ map) that can be used without disk
 * access. The map resembles a CPU cache line.
 *
 * The stack is responsible for flushing this to disk at regular intervals to keep the map size
 * small and guard against loss of data in the event of system failure. Sets and gets will fill
 * this map.
 *
 */
template <typename T, typename D = uint64_t>
class CacheLineRandomAccessStack
{
public:
  using event_handler_type = std::function<void()>;
  using stack_type         = RandomAccessStack<T, D>;
  using header_extra_type  = D;
  using type               = T;

  CacheLineRandomAccessStack() = default;

  ~CacheLineRandomAccessStack()
  {
    Flush(false);
  }

  /**
   * Indicate whether the stack is writing directly to disk or caching writes. Since
   * This class intends to invisibly provide caching it returns that it's a
   * direct write class.
   *
   * @return: Whether the stack is written straight to disk.
   */
  static constexpr bool DirectWrite()
  {
    return true;
  }

  void Load(std::string const &filename, bool const &create_if_not_exists = true)
  {
    stack_.Load(filename, create_if_not_exists);
    this->objects_ = stack_.size();
    this->SignalFileLoaded();
  }

  void New(std::string const &filename)
  {
    stack_.New(filename);
    this->objects_ = 0;
    this->SignalFileLoaded();
  }

  void ClearEventHandlers()
  {
    on_file_loaded_  = nullptr;
    on_before_flush_ = nullptr;
  }

  void OnFileLoaded(event_handler_type const &f)
  {
    on_file_loaded_ = f;
  }

  void OnBeforeFlush(event_handler_type const &f)
  {
    on_before_flush_ = f;
  }

  void Get(uint64_t const &i, type &object) const
  {
    assert(i < objects_);

    uint64_t cache_lookup   = i >> cache_line_ln2;              // Upper N bits
    uint64_t cache_subindex = i & ((1 << cache_line_ln2) - 1);  // Lower total - N bits

    auto iter = data_.find(cache_lookup);

    // Found the item via local map
    if (iter != data_.end())
    {
      ++((*iter).second.reads);
      object = iter->second.elements[cache_subindex];
    }
    else
    {
      // Case where item isn't found, load it into the cache, then access
      LoadCacheLine(i);

      data_[cache_lookup].reads++;
      object = data_[cache_lookup].elements[cache_subindex];
    }
  }

  /**
   * Set index i to object. Undefined behaviour if i >= stack size.
   *
   * @param: i The index
   * @param: object The object to write
   */
  void Set(uint64_t const &i, type const &object)
  {
    assert(i < objects_);

    uint64_t cache_lookup   = i >> cache_line_ln2;              // Upper N bits
    uint64_t cache_subindex = i & ((1 << cache_line_ln2) - 1);  // Lower total - N bits

    auto iter = data_.find(cache_lookup);

    // Found the item via local map
    if (iter != data_.end())
    {
      ++iter->second.writes;
      iter->second.elements[cache_subindex] = object;
    }
    else
    {
      // Case where item isn't found, load it into the cache, then access
      LoadCacheLine(i);

      data_[cache_lookup].writes++;
      data_[cache_lookup].elements[cache_subindex] = object;
    }
  }

  void Close()
  {
    Flush(false);
    stack_.Close(false);
  }

  void SetExtraHeader(header_extra_type const &he)
  {
    stack_.SetExtraHeader(he);
  }

  header_extra_type const &header_extra() const
  {
    return stack_.header_extra();
  }

  uint64_t Push(type const &object)
  {
    uint64_t ret = objects_;

    ++objects_;

    // Guaranteed to be safe - sets make sure the underlying stack has the memory
    Set(objects_ - 1, object);

    return ret;
  }

  /**
   * Since we're caching, we decrement our internal counter and fix it on the next hard
   * flush.
   */
  void Pop()
  {
    --objects_;
  }

  type Top()
  {
    type ret;
    Get(objects_ - 1, ret);
    return ret;
  }

  void Swap(uint64_t const &i, uint64_t const &j)
  {
    if (i == j)
    {
      return;
    }

    uint64_t cache_lookup_i = i >> cache_line_ln2;
    uint64_t cache_lookup_j = j >> cache_line_ln2;

    uint64_t cache_subindex_i = i & ((1 << cache_line_ln2) - 1);
    uint64_t cache_subindex_j = j & ((1 << cache_line_ln2) - 1);

    // make sure both items are in the cache
    if (data_.find(cache_lookup_i) == data_.end())
    {
      LoadCacheLine(cache_lookup_i);
    }

    if (data_.find(cache_lookup_j) == data_.end())
    {
      LoadCacheLine(cache_lookup_j);
    }

    // If this assertion fails, there might not be enough memory for two cache lines
    assert(data_.find(cache_lookup_i) != data_.end());
    assert(data_.find(cache_lookup_j) != data_.end());

    data_[cache_lookup_i].reads++;
    data_[cache_lookup_j].reads++;

    std::swap(data_[cache_lookup_i].elements[cache_subindex_i],
              data_[cache_lookup_j].elements[cache_subindex_j]);
  }

  std::size_t size() const
  {
    return objects_;
  }

  std::size_t empty() const
  {
    return objects_ == 0;
  }

  void Clear()
  {
    stack_.Clear();
    objects_ = 0;
    data_.clear();
  }

  /**
   * Flush all of the cached elements to file if they have been updated
   */
  void Flush(bool lazy = true) const
  {
    if (!lazy)
    {
      for (auto const &i : data_)
      {
        FlushLine(i.first << cache_line_ln2, i.second);
      }

      // Trim stack size down
      while (stack_.size() > objects_ && stack_.is_open())
      {
        stack_.Pop();
      }

      if (stack_.is_open())
      {
        stack_.Flush(false);
      }
    }
  }

  bool is_open() const
  {
    return stack_.is_open();
  }

  /**
   * Set the limit for the amount of RAM this structure will use to amortize the cost of disk
   * writes.
   *
   * @param: bytes The number of bytes allowed as an upper bound
   */
  void SetMemoryLimit(std::size_t bytes)
  {
    memory_limit_bytes_ = bytes;
  }

private:
  // Cached items
  static constexpr std::size_t cache_line_ln2 = 13;  // Default cache lines 8192 * sizeof(T)
  std::size_t memory_limit_bytes_             = std::size_t(1ULL << 29);  // Default 500K memory

  T dummy_;

  event_handler_type on_file_loaded_;
  event_handler_type on_before_flush_;

  // Underlying stack
  mutable stack_type stack_;

  struct CachedDataItem
  {
    uint64_t                              reads  = 0;
    uint64_t                              writes = 0;
    std::array<type, 1 << cache_line_ln2> elements;
  };

  mutable std::map<uint64_t, CachedDataItem> data_;
  mutable uint64_t                           last_removed_index_ = 0;
  uint64_t                                   objects_            = 0;

  void FlushLine(uint64_t line, CachedDataItem const &items) const
  {
    if (items.writes == 0)
    {
      return;
    }

    if (!stack_.is_open())
    {
      return;
    }

    stack_.SetBulk(line, 1ull << cache_line_ln2, items.elements.data());
  }

  void GetLine(uint64_t line, CachedDataItem &items) const
  {
    if (!stack_.is_open())
    {
      return;
    }

    stack_.GetBulk(line, 1 << cache_line_ln2, items.elements.data());
  }

  bool ManageMemory() const
  {
    // Lazy policy: remove items FILO style
    using MapElement = typename decltype(data_)::value_type;

    if (!(data_.size() * sizeof(MapElement) > memory_limit_bytes_))
    {
      return false;
    }

    // Find and remove next index up from the last one we removed
    auto next_to_remove = data_.upper_bound(last_removed_index_);

    if (next_to_remove->first > last_removed_index_ && next_to_remove != data_.end())
    {
      last_removed_index_ = next_to_remove->first;
      FlushLine(next_to_remove->first << cache_line_ln2, next_to_remove->second);
      data_.erase(next_to_remove);
    }
    else
    {
      next_to_remove = data_.begin();  // Get min element
      FlushLine(next_to_remove->first << cache_line_ln2, next_to_remove->second);
      last_removed_index_ = next_to_remove->first;

      data_.erase(next_to_remove);
    }

    return true;
  }

  void LoadCacheLine(uint64_t line) const
  {
    // Cull memory usage to max allowed
    for (;;)
    {
      if (!ManageMemory())
      {
        break;
      }
    }

    // Load in the cache line (memory usage now slightly over)
    uint64_t cache_index = (line >> cache_line_ln2) << cache_line_ln2;
    GetLine(cache_index, data_[cache_index >> cache_line_ln2]);
  }

  void SignalFileLoaded()
  {
    if (on_file_loaded_)
    {
      on_file_loaded_();
    }
  }
};
}  // namespace storage
}  // namespace fetch
