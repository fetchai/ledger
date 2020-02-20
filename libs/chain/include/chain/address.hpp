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
#include "core/serialisers/main_serialiser.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace fetch {
namespace crypto {
class Identity;
}

namespace chain {

/**
 * The Address is a unifying mechanism between multiple different public keys types into a single
 * format.
 *
 * It is generated by creating a hash of the raw bytes of the public key. In addition, when
 * displaying the address on interfaces an additional 4 bytes of checksum is appended to the
 * address. This checksum is calculated by hashing the address and selecting the first 4 bytes of
 * the digest. This mechanism is common place and allows interfaces to integrity check the address.
 *
 * ┌──────────────────────────────────────────────────┐
 * │                    Public Key                    │
 * └──────────────────────────────────────────────────┘
 *                           │
 *                           │
 *                   Hashed (SHA-256)
 *                           │
 *                           │
 *                           ▼
 *                ┌─────────────────────┐
 *                │       Address       │ ───────────┐
 *                └─────────────────────┘            │
 *                           │                       │
 *                           │                       │
 *                    When Displaying              1st 4
 *                           │                    bytes of
 *                           │                      the
 *                           ▼                       │
 *            ┌─────────────────────┬────────┐       │
 *            │       Address       │Checksum│ ◀─────┘
 *            └─────────────────────┴────────┘
 *
 */
class Address
{
public:
  static constexpr std::size_t RAW_LENGTH      = 32;
  static constexpr std::size_t CHECKSUM_LENGTH = 4;
  static constexpr std::size_t TOTAL_LENGTH    = RAW_LENGTH + CHECKSUM_LENGTH;

  using RawAddress     = std::array<uint8_t, RAW_LENGTH>;
  using ConstByteArray = byte_array::ConstByteArray;

  // Helpers
  static bool    Parse(ConstByteArray const &input, Address &output);
  static Address FromMuddleAddress(ConstByteArray const &muddle);

  // Construction / Destruction
  Address() = default;
  explicit Address(crypto::Identity const &identity);
  explicit Address(RawAddress const &address);
  explicit Address(ConstByteArray address);
  Address(Address const &) = default;
  Address(Address &&)      = default;
  ~Address()               = default;

  /// @name Accessors
  /// @{
  ConstByteArray const &address() const;
  ConstByteArray const &display() const;
  bool                  empty() const;
  /// @}

  // Operators
  bool operator==(Address const &other) const;
  bool operator!=(Address const &other) const;
  bool operator<(Address const &other) const;
  bool operator<=(Address const &other) const;
  bool operator>(Address const &other) const;
  bool operator>=(Address const &other) const;

  Address &operator=(Address const &) = default;
  Address &operator=(Address &&) = default;

private:
  ConstByteArray address_;  ///< The address representation
  ConstByteArray display_;  ///< The display representation
};

}  // namespace chain

namespace serializers {

template <typename D>
struct MapSerializer<chain::Address, D>  // TODO(issue 1422): Use forward to bytearray
{
public:
  using Type       = chain::Address;
  using DriverType = D;

  static uint8_t const ADDRESS = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &data)
  {
    auto map = map_constructor(1);
    map.Append(ADDRESS, data.address());
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &address)
  {
    uint8_t                    key;
    byte_array::ConstByteArray data;
    map.GetNextKeyPair(key, data);
    address = data.empty() ? Type{} : Type{data};
  }
};
}  // namespace serializers

}  // namespace fetch

namespace std {

template <>
struct hash<fetch::chain::Address>
{
  std::size_t operator()(fetch::chain::Address const &address) const noexcept
  {
    auto const &raw_address = address.address();

    if (raw_address.empty())
    {
      return 0;
    }

    return *reinterpret_cast<std::size_t const *>(raw_address.pointer());
  }
};

}  // namespace std
