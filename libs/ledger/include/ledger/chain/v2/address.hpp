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

#include <array>

namespace fetch {
namespace crypto {
  class Identity;
}

namespace ledger {
namespace v2 {

class Address
{
public:
  using RawAddress = std::array<uint8_t, 32>;
  using ConstByteArray = byte_array::ConstByteArray;

  // Construction / Destruction
  Address() = default;
  explicit Address(crypto::Identity const &identity);
  explicit Address(RawAddress const &address);
  Address(Address const &) = default;
  Address(Address &&) = default;
  ~Address() = default;

  ConstByteArray address() const;
  ConstByteArray display() const;

  // Operators
  bool operator==(Address const &other) const;
  bool operator!=(Address const &other) const;

  Address &operator=(Address const &) = default;
  Address &operator=(Address &&) = default;

private:

  ConstByteArray address_;
  ConstByteArray display_;
};

inline Address::ConstByteArray Address::address() const
{
  return address_;
}

inline Address::ConstByteArray Address::display() const
{
  return display_;
}

inline bool Address::operator==(Address const &other) const
{
  return address_ == other.address_;
}

inline bool Address::operator!=(Address const &other) const
{
  return !(*this == other);
}

} // namespace v2
} // namespace ledger
} // namespace fetch
