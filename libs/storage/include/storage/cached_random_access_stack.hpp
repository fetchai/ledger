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

#include "core/assert.hpp"
#include "storage/random_access_stack.hpp"

#include <fstream>
#include <map>
#include <string>
#include <unordered_map>

namespace fetch {
namespace storage {

template <typename T, typename D = uint64_t>
class CachedRandomAccessStack
{
public:
  using event_handler_type = std::function<void()>;
  using stack_type         = RandomAccessStack<T, D>;
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

  ~CachedRandomAccessStack() { stack_.ClearEventHandlers(); }

  void ClearEventHandlers()
  {
    on_file_loaded_  = nullptr;
    on_before_flush_ = nullptr;
  }

  void OnFileLoaded(event_handler_type const &f) { on_file_loaded_ = f; }

  void OnBeforeFlush(event_handler_type const &f) { on_before_flush_ = f; }

  // TODO(issue 13): Move private or protected
  void SignalFileLoaded()
  {
    if (on_file_loaded_) on_file_loaded_();
  }

  void SignalBeforeFlush()
  {
    if (on_before_flush_) on_before_flush_();
  }

  static constexpr bool DirectWrite() { return false; }

  void Load(std::string const &filename, bool const &create_if_not_exists = true)
  {
    stack_.Load(filename, create_if_not_exists);
    total_access_ = 0;
    this->SignalFileLoaded();
  }

  void New(std::string const &filename)
  {
    stack_.New(filename);
    Clear();
    total_access_ = 0;
    this->SignalFileLoaded();
  }

  void Get(uint64_t const &i, type &object) const
  {
    assert(i < objects_);
    ++total_access_;

    auto iter = data_.find(i);
    if (iter != data_.end())
    {
      ++iter->second.reads;
      object = iter->second.data;
    }
    else
    {
      stack_.Get(i, object);
      CachedDataItem itm;
      itm.data = object;
      data_.insert(std::pair<uint64_t, CachedDataItem>(i, itm));
    }
  }

  void Set(uint64_t const &i, type const &object)
  {
    ++total_access_;

    auto iter = data_.find(i);
    if (iter != data_.end())
    {
      ++iter->second.writes;
      iter->second.updated = true;
      iter->second.data    = object;
    }
    else
    {
      assert(false);
      assert(i < objects_);
      CachedDataItem itm;
      itm.data = object;
      data_.insert(std::pair<uint64_t, CachedDataItem>(i, itm));
    }
  }

  void Close()
  {
    Flush();

    stack_.Close(true);
  }

  void SetExtraHeader(header_extra_type const &he) { stack_.SetExtraHeader(he); }

  header_extra_type const &header_extra() const { return stack_.header_extra(); }

  uint64_t Push(type const &object)
  {
    ++total_access_;
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

  type Top() const { return data_[objects_ - 1]; }

  void Swap(uint64_t const &i, uint64_t const &j)
  {
    if (i == j) return;
    total_access_ += 2;
    data_.find(i)->second.updated = true;
    data_.find(j)->second.updated = true;
    std::swap(data_[i], data_[j]);
  }

  std::size_t size() const { return objects_; }
  std::size_t empty() const { return objects_ == 0; }

  void Clear()
  {
    stack_.Clear();
    objects_ = 0;
    data_.clear();
  }

  void Flush()
  {
    this->SignalBeforeFlush();

    for (auto &item : data_)
    {
      if (item.second.updated)
      {
        if (item.first >= stack_.size())
        {
          assert(item.first == stack_.size());
          stack_.LazyPush(item.second.data);
        }
        else
        {
          assert(item.first < stack_.size());
          stack_.Set(item.first, item.second.data);
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
    total_access_ = 0;
    // TODO(issue 10): Clear those not needed
  }

  bool is_open() const { return stack_.is_open(); }

private:
  event_handler_type on_file_loaded_;
  event_handler_type on_before_flush_;

  stack_type       stack_;
  mutable uint64_t total_access_;
  struct CachedDataItem
  {
    uint64_t reads   = 0;
    uint64_t writes  = 0;
    bool     updated = false;
    type     data;
  };

  mutable std::map<uint64_t, CachedDataItem> data_;
  uint64_t                                   objects_ = 0;
};
}  // namespace storage
}  // namespace fetch
