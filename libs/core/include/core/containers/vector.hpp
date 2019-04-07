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

#include <vector>

namespace fetch {
namespace containers {

template <typename T>
class Vector : private std::vector<T>
{
public:
  using super_type = std::vector<T>;
  using type       = T;

  type &At(std::size_t const i)
  {
    return super_type::at(i);
  }

  type const &At(std::size_t const i) const
  {
    return super_type::at(i);
  }

  type &operator[](std::size_t const i)
  {
    return super_type::at(i);
  }
  type const &operator[](std::size_t const i) const
  {
    return super_type::at(i);
  }

  type &Front()
  {
    return super_type::front();
  }
  type const &Front() const
  {
    return super_type::front();
  }

  type &Back()
  {
    return super_type::back();
  }
  type const &Back() const
  {
    return super_type::back();
  }

  void Clear()
  {
    return super_type::clear();
  }

  void Insert(type const &element)
  {}

  void Erase(std::size_t pos)
  {}

  void PushBack(type const &element)
  {
    super_type::push_back(element);
  }

  void PopBack()
  {
    super_type::pop_back();
  }

  void Resize(std::size_t n)
  {
    super_type::resize(n);
  }

  void Reserve(std::size_t const n)
  {
    return super_type::reserve(n);
  }

  bool empty() const
  {
    return super_type::empty();
  }

  std::size_t size() const
  {
    return super_type::size();
  }

  std::size_t capacity() const
  {
    return super_type::capacity();
  }

  void swap(Vector &other)
  {}
};
}  // namespace containers
}  // namespace fetch
