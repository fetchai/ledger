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
#include "core/serializers/group_definitions.hpp"
#include "crypto/sha1.hpp"
#include "muddle/packet.hpp"
#include "vectorise/platform.hpp"

namespace fetch {
namespace muddle {

struct KademliaAddress
{
  using Address = Packet::Address;
  using Hasher  = crypto::SHA1;

  enum
  {
    ADDRESS_SIZE = Hasher::SIZE_IN_BYTES
  };

  KademliaAddress()
  {
    for (auto &word : words)
    {
      word = 0;
    }
  }

  static KademliaAddress Create(Address const &address)
  {

    KademliaAddress ret;
    Hasher          hasher;
    hasher.Update(address);

    hasher.Final(ret.words);

    return ret;
  }

  static KademliaAddress FromByteArray(byte_array::ConstByteArray const &address)
  {
    KademliaAddress ret;

    if (address.size() != ADDRESS_SIZE)
    {
      throw std::runtime_error("Kademlia address size mismatch.");
    }

    for (uint64_t i = 0; i < ADDRESS_SIZE; ++i)
    {
      ret.words[i] = address[i];
    }

    return ret;
  }

  constexpr uint64_t size() const
  {
    return ADDRESS_SIZE;
  }

  byte_array::ByteArray ToByteArray() const
  {
    byte_array::ByteArray ret;
    ret.Resize(ADDRESS_SIZE);

    for (uint64_t i = 0; i < ret.size(); ++i)
    {
      ret[i] = words[i];
    }

    return ret;
  }

  uint8_t words[ADDRESS_SIZE];
};

using KademliaDistance = std::array<uint8_t, KademliaAddress::ADDRESS_SIZE>;

inline KademliaDistance GetKademliaDistance(KademliaAddress const &a, KademliaAddress const &b)
{
  KademliaDistance ret;

  for (std::size_t i = 0; i < a.size(); ++i)
  {
    ret[i] = a.words[i] ^ b.words[i];
  }

  return ret;
}

}  // namespace muddle

namespace serializers {

template <typename D>
struct ForwardSerializer<muddle::KademliaAddress, D>
{
public:
  using Type       = muddle::KademliaAddress;
  using DriverType = D;

  template <typename Serializer>
  static void Serialize(Serializer &serializer, Type const &adr)
  {
    serializer << adr.ToByteArray();
  }

  template <typename Deserializer>
  static void Deserialize(Deserializer &deserializer, Type &adr)
  {
    byte_array::ConstByteArray a;
    deserializer >> a;
    adr = Type::FromByteArray(a);
  }
};

}  // namespace serializers

}  // namespace fetch
