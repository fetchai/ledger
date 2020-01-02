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

#include "core/byte_array/const_byte_array.hpp"

#include <cstddef>
#include <utility>

namespace fetch {
namespace byte_array {

class ByteArray : public ConstByteArray
{
public:
  using SelfType  = ByteArray;
  using SuperType = ConstByteArray;
  using SuperType::Reserve;
  using SuperType::Resize;
  using SuperType::SuperType;
  using SuperType::operator+;
  using SuperType::operator[];
  using SuperType::Append;
  using SuperType::char_pointer;
  using SuperType::pointer;
  using SuperType::Replace;
  using SuperType::SubArray;

  ByteArray() = default;

  ByteArray(SuperType const &other)  // NOLINT
    : SuperType(other.Copy())
  {}
  ByteArray(SuperType &&other)  // NOLINT
    : SuperType(other.IsUnique() ? std::move(other) : other.Copy())
  {}

  SelfType SubArray(std::size_t start, std::size_t length = std::size_t(-1)) const
  {
    return SubArrayInternal<SelfType>(start, length);
  }
};

}  // namespace byte_array
}  // namespace fetch
