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

#include <unordered_map>
#include "storage/storage_exception.hpp"

template <typename STACK_ELEMENTS>
class FakeRandomAccessStack
{
public:
  using type               = STACK_ELEMENTS;

  using StorageException = fetch::storage::StorageException;

  ~FakeRandomAccessStack() = default;

  void Load(std::string const &filename, bool const &create_if_not_exist = false)
  {
    FETCH_UNUSED(create_if_not_exist);

    stack_ = &underlying_stack_[filename];
    is_open_ = true;
  }

  void New(std::string const &filename)
  {
    underlying_stack_[filename] = FakeStack<STACK_ELEMENTS>{};
    stack_ = &underlying_stack_[filename];
    is_open_ = true;
  }

  void Get(std::size_t const &i, STACK_ELEMENTS &object) const
  {
    ThrowOnBadAccess(i, "Get");
    object = (*stack_).elements[i];
  }

  void Set(std::size_t const &i, STACK_ELEMENTS const &object)
  {
    ThrowOnBadAccess(i, "Set");
    (*stack_).elements[i] = object;
  }

  void SetBulk(std::size_t const &i, std::size_t elements, STACK_ELEMENTS const *objects)
  {
    for (std::size_t increment = 0; increment < elements; ++increment)
    {
      ThrowOnBadAccess(i + increment, "SetBulk");
      Set(i + increment, objects[i]);
    }
  }

  bool LazySetBulk(std::size_t const &i, std::size_t elements, STACK_ELEMENTS const *objects)
  {
    ThrowOnBadAccess(i + elements, "LazySetBulk");
    SetBulk(i, elements, objects);
  }

  void GetBulk(std::size_t const &i, std::size_t elements, STACK_ELEMENTS *objects)
  {
    for (std::size_t increment = 0; increment < elements; ++increment)
    {
      ThrowOnBadAccess(i + increment, "GetBulk");
      Set(i + increment, objects[i]);
    }
  }

  uint64_t Push(STACK_ELEMENTS const &object)
  {
    ThrowOnBadAccess("Push");
    (*stack_).elements.push_back(object);
    return size();
  }

  void Pop()
  {
    ThrowOnBadAccess(0, "Pop");
    (*stack_).elements.pop_back();
  }

  STACK_ELEMENTS Top() const
  {
    ThrowOnBadAccess(0, "Top");
    return (*stack_).elements[(*stack_).elements.size()-1];
  }

  void SetExtraHeader(uint64_t const &he)
  {
    (*stack_).header = he;
  }

  uint64_t const &header_extra() const
  {
    return (*stack_).header;
  }

  std::size_t size() const
  {
    ThrowOnBadAccess("size");
    return (*stack_).elements.size();
  }

  std::size_t empty() const
  {
    ThrowOnBadAccess("empty");
    return (*stack_).elements.empty();
  }

  void Clear()
  {
    ThrowOnBadAccess("Clear");
    (*stack_).elements.clear();
  }

  bool is_open() const
  {
    return is_open_;
  }

  void Swap(std::size_t const &i, std::size_t const &j)
  {
    ThrowOnBadAccess(i, "Swap");
    ThrowOnBadAccess(j, "Swap");
    std::swap((*stack_).elements[i], (*stack_).elements[i]);
  }

  uint64_t LazyPush(STACK_ELEMENTS const &object)
  {
    ThrowOnBadAccess("LazyPush");
    Push(object);
  }

  std::fstream &underlying_stream()
  {
    throw StorageException("Not testable.");
  }

  // Unused functions
  void ClearEventHandlers()
  {
  }

  // TODO(HUT): delete these!
  /*
  void OnFileLoaded(event_handler_type const &f)
  {
  }

  void OnBeforeFlush(event_handler_type const &f)
  {
  }*/

  void SignalFileLoaded()
  {
  }

  void SignalBeforeFlush()
  {
  }

  static constexpr bool DirectWrite()
  {
    return true;
  }

  void Close(bool const &lazy = false)
  {
    FETCH_UNUSED(lazy);
    is_open_ = false;
  }

  void Flush(bool const &lazy = false)
  {
    FETCH_UNUSED(lazy);
  }

private:

  void ThrowOnBadAccess(std::string const &fn_name) const
  {
    if(!is_open_)
    {
      throw StorageException("attempt to use closed fake RAS in fn: " + fn_name);
    }
  }

  void ThrowOnBadAccess(std::size_t const &i, std::string const &fn_name) const
  {
    if(!is_open_)
    {
      throw StorageException("attempt to use closed fake RAS in fn: " + fn_name);
    }

    if(i >= (*stack_).elements.size())
    {
      throw StorageException("index out of bounds stack in fake RAS fn: " + fn_name);
    }
  }

  bool is_open_ = false;

  template <typename T>
  struct FakeStack
  {
    uint64_t       header = 0;
    std::vector<T> elements;
  };

  FakeStack<STACK_ELEMENTS> *stack_ = nullptr;

  std::unordered_map<std::string, FakeStack<STACK_ELEMENTS>> underlying_stack_;
};
