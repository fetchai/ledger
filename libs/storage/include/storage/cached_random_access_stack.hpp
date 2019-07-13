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

//  ┌──────┬───────────┬───────────┬───────────┬───────────┐
//  │      │           │           │           │           │
//  │HEADER│  OBJECT   │  OBJECT   │  OBJECT   │  OBJECT   │
//  │      │           │           │           │           │......
//  │      │           │           │           │           │
//  └──────┴───────────┴───────────┴───────────┴───────────┘

#include "core/assert.hpp"
#include "storage/random_access_stack.hpp"

#include <fstream>
#include <map>
#include <string>
#include <unordered_map>

namespace fetch {
namespace storage {

/**
 * The CachedRandomAccessStack owns a stack of type T (RandomAccessStack), and provides caching.
 *
 * It does this by maintaining a quick access structure (data_ map) that can be used without disk
 * access.
 *
 * The user is responsible for flushing this to disk at regular intervals to keep the map size
 * small and guard against loss of data in the event of system failure. Sets and gets will fill
 * this map.
 *
 */
template <typename T, typename D = uint64_t, typename STACK = RandomAccessStack<T, D>>
class CachedRandomAccessStack
{
public:
  using event_handler_type = std::function<void()>;
  using stack_type         = STACK;
  using header_extra_type  = D;
  using type               = T;

  CachedRandomAccessStack()
  {
    stack_.OnFileLoaded([this]() {
      this->objects_ = stack_.size();
      SignalFileLoaded();
    });
    stack_.OnBeforeFlush([this]() { SignalBeforeFlush(); });
  }

  ~CachedRandomAccessStack()
  {
    stack_.ClearEventHandlers();
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

  /**
   * Indicate whether the stack is writing directly to disk or caching writes.
   *
   * @return: Whether the stack is written straight to disk.
   */
  static constexpr bool DirectWrite()
  {
    return false;
  }

  void Load(std::string const &filename, bool const &create_if_not_exists = true)
  {
    stack_.Load(filename, create_if_not_exists);
    this->SignalFileLoaded();
  }

  void New(std::string const &filename)
  {
    stack_.New(filename);
    Clear();
    this->SignalFileLoaded();
  }

  void Get(uint64_t i, type &object) const
  {
    assert(i < objects_);

    auto iter = data_.find(i);

    // Found the item via local map
    if (iter != data_.end())
    {
      ++iter->second.reads;
      object = iter->second.data;
    }
    else
    {
      // Case where item isn't found, get it from the stack and insert it into the map
      stack_.Get(i, object);
      CachedDataItem itm;
      itm.data = object;
      data_.insert(std::pair<uint64_t, CachedDataItem>(i, itm));
    }
  }

  /**
   * Set index i to object. Undefined behaviour if i >= stack size.
   *
   * @param: i The index
   * @param: object The object to write
   */
  void Set(uint64_t i, type const &object)
  {
    assert(i < objects_);

    auto iter = data_.find(i);
    if (iter != data_.end())
    {
      auto &cached_element = iter->second;
      cached_element.writes++;
      cached_element.updated = true;
      cached_element.data    = object;
    }
    else
    {
      CachedDataItem itm;
      itm.data    = object;
      itm.updated = true;
      itm.writes  = 1;
      data_.insert(std::pair<uint64_t, CachedDataItem>(i, itm));
    }
  }

  void Close()
  {
    Flush();

    stack_.Close(true);
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
    uint64_t       ret = objects_;
    CachedDataItem itm;
    itm.data    = object;
    itm.updated = true;
    data_.insert(std::pair<uint64_t, CachedDataItem>(ret, itm));
    ++objects_;

    return ret;
  }

  void Pop()
  {
    --objects_;
    data_.erase(objects_);
  }

  type Top() const
  {
    type ret;
    Get(objects_ - 1, ret);
    return ret;
  }

  void Swap(uint64_t i, uint64_t j)
  {

    if (i == j)
    {
      return;
    }

    data_.find(i)->second.updated = true;
    data_.find(j)->second.updated = true;
    std::swap(data_[i], data_[j]);
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
  void Flush(bool /*lazy*/ = true)
  {
    this->SignalBeforeFlush();

    for (auto &item : data_)
    {
      auto const &index          = item.first;
      auto const &cached_element = item.second;

      if (cached_element.updated)
      {
        // In the case the stack size is less than the index we're trying to write this means we
        // need to push onto the stack. This is unsafe if we have non-continuous iteration of index
        if (index >= stack_.size())
        {
          assert(index == stack_.size());
          stack_.LazyPush(cached_element.data);
        }
        else
        {
          assert(index < stack_.size());
          stack_.Set(index, cached_element.data);
        }
      }
    }

    stack_.Flush(true);

    for (auto &item : data_)
    {
      item.second.reads   = 0;
      item.second.writes  = 0;
      item.second.updated = false;
    }

    // TODO(issue 10): Manage cache size
  }

  bool is_open() const
  {
    return stack_.is_open();
  }

  stack_type &underlying_stack()
  {
    return stack_;
  }

private:
  static constexpr std::size_t MAX_SIZE_BYTES = 10000;
  event_handler_type           on_file_loaded_;
  event_handler_type           on_before_flush_;

  // Underlying stack
  stack_type stack_;

  // Cached items
  struct CachedDataItem
  {
    uint64_t reads   = 0;
    uint64_t writes  = 0;
    bool     updated = false;
    type     data;
  };

  mutable std::map<uint64_t, CachedDataItem> data_;
  uint64_t                                   objects_ = 0;

  // TODO(issue 13): Move private or protected
  void SignalFileLoaded()
  {
    if (on_file_loaded_)
    {
      on_file_loaded_();
    }
  }

  /**
   * Call the closure before any flush
   */
  void SignalBeforeFlush()
  {
    if (on_before_flush_)
    {
      on_before_flush_();
    }
  }
};
}  // namespace storage
}  // namespace fetch
