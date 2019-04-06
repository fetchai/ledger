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

#include "core/byte_array/const_byte_array.hpp"

#include <map>
namespace fetch {
namespace http {

class KeyValueSet : private std::map<byte_array::ConstByteArray, byte_array::ConstByteArray>
{
public:
  using super_type      = std::map<byte_array::ConstByteArray, byte_array::ConstByteArray>;
  using byte_array_type = byte_array::ConstByteArray;
  using map_type        = std::map<byte_array_type, byte_array_type>;
  using iterator        = map_type::iterator;
  using const_iterator  = map_type::const_iterator;

  iterator begin() noexcept
  {
    return map_type::begin();
  }
  iterator end() noexcept
  {
    return map_type::end();
  }
  const_iterator begin() const noexcept
  {
    return map_type::begin();
  }
  const_iterator end() const noexcept
  {
    return map_type::end();
  }
  const_iterator cbegin() const noexcept
  {
    return map_type::cbegin();
  }
  const_iterator cend() const noexcept
  {
    return map_type::cend();
  }

  void Add(byte_array_type const &name, byte_array_type const &value)
  {
    LOG_STACK_TRACE_POINT;

    insert({name, value});
  }

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value, void>::type Add(byte_array_type const &name,
                                                                      T const &              n)
  {
    LOG_STACK_TRACE_POINT;
    // TODO(issue 35): Can be improved.
    byte_array_type value(std::to_string(n));
    insert({name, value});
  }

  bool Has(byte_array_type const &key) const
  {
    LOG_STACK_TRACE_POINT;

    return this->find(key) != this->end();
  }

  byte_array::ConstByteArray &operator[](byte_array::ConstByteArray const &name)
  {
    LOG_STACK_TRACE_POINT;

    return super_type::operator[](name);
  }

  byte_array::ConstByteArray operator[](byte_array::ConstByteArray const &name) const
  {
    LOG_STACK_TRACE_POINT;

    auto element = this->find(name);

    if (element == this->end())
    {
      return {};
    }
    else
    {
      return element->second;
    }
  }

  void Clear()
  {
    LOG_STACK_TRACE_POINT;

    this->clear();
  }

private:
};
}  // namespace http
}  // namespace fetch
