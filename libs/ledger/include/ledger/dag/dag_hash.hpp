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

#include "chain/common_types.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/main_serializer.hpp"

namespace fetch {

namespace ledger {

struct DAGHash
{
  using ConstByteArray = byte_array::ConstByteArray;

  enum class Type : int8_t
  {
    NODE = 0,
    EPOCH
  };

  DAGHash();
  explicit DAGHash(Type t);
  explicit DAGHash(ConstByteArray h);
  DAGHash(ConstByteArray h, Type t);

  explicit operator ConstByteArray();
  explicit operator ConstByteArray() const;

  bool empty() const;

  bool operator==(DAGHash const &other) const;
  bool operator>(DAGHash const &rhs) const;
  bool operator<(DAGHash const &rhs) const;

  bool IsEpoch() const;
  bool IsNode() const;

  ConstByteArray ToBase64() const;
  ConstByteArray ToHex() const;

  ConstByteArray hash;
  Type           type = Type::NODE;
};
}  // namespace ledger

namespace serializers {

template <typename D>
struct MapSerializer<ledger::DAGHash, D>
{
public:
  using Type       = ledger::DAGHash;
  using DriverType = D;

  static uint8_t const HASH = 1;
  static uint8_t const TYPE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &node)
  {
    auto map = map_constructor(2);
    map.Append(HASH, node.hash);
    map.Append(TYPE, static_cast<int8_t>(node.type));
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &node)
  {
    map.ExpectKeyGetValue(HASH, node.hash);
    int8_t type;
    map.ExpectKeyGetValue(TYPE, type);
    node.type = static_cast<Type::Type>(node.type);
  }
};
}  // namespace serializers

}  // namespace fetch

namespace std {
template <>
struct hash<fetch::ledger::DAGHash>
{
  std::size_t operator()(fetch::ledger::DAGHash const &value) const noexcept
  {
    return std::hash<fetch::byte_array::ConstByteArray>()(value.hash);
  }
};
}  // namespace std
