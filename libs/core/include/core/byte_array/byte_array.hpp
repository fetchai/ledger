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

#include <algorithm>
#include <cassert>
#include <iostream>
#include <ostream>
#include <type_traits>

namespace fetch {
namespace byte_array {

class ByteArray : public ConstByteArray
{
public:
  using self_type  = ByteArray;
  using super_type = ConstByteArray;
  using super_type::super_type;
  using super_type::Resize;
  using super_type::Reserve;
  using super_type::operator+;
  using super_type::operator[];
  using super_type::pointer;
  using super_type::char_pointer;
  using super_type::SubArray;
  using super_type::Append;
  using super_type::Replace;

  ByteArray() = default;

  ByteArray(super_type const &other)
    : super_type(other.Copy())
  {}
  ByteArray(super_type &&other)
    : super_type(other.IsUnique() ? std::move(other) : other.Copy())
  {}

  self_type SubArray(std::size_t const &start, std::size_t length = std::size_t(-1)) const
  {
    return SubArray<self_type>(start, length);
  }
};

}  // namespace byte_array
}  // namespace fetch
