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

namespace fetch {

namespace ledger {

struct DAGHash {
  using ConstByteArray = byte_array::ConstByteArray;

  DAGHash() = default;

  explicit DAGHash(ConstByteArray h)
      : hash{std::move(h)}
  {
  }

  explicit DAGHash(ConstByteArray h, char t)
      : hash{std::move(h)}
      , type{t}
  {
  }

  explicit operator ConstByteArray() {
    return hash;
  }

  bool empty() const
  {
    return hash.empty();
  }

  bool operator==(DAGHash const &other) const {
    return hash == other.hash;
  }

  bool operator>(DAGHash const &rhs) const
  {
    return hash > rhs.hash;
  }

  bool operator<(DAGHash const &rhs) const
  {
    return hash < rhs.hash;
  }

  bool IsEpoch() const
  {
    return type == 'E';
  }

  bool IsNode() const
  {
    return type == 'N';
  }

  ConstByteArray ToBase64() const
  {
    return hash.ToBase64()+(std::string("/") + type);
  }

  ConstByteArray ToHex() const
  {
    return (hash+(std::string("/")+type)).ToHex();
  }

  ConstByteArray hash;
  char type = 'N';
};
}

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
    node.type = static_cast<char>(node.type);
  }
};
}  // namespace serializers

}

namespace std
{
  template<>
  struct hash<fetch::ledger::DAGHash>
  {
    std::size_t operator()(fetch::ledger::DAGHash const &value) const noexcept
    {
      return std::hash<fetch::byte_array::ConstByteArray>()(value.hash);
    }
  };
}