#pragma once
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
    for (uint64_t i = 0; i < ADDRESS_SIZE; ++i)
    {
      words[i] = 0;
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

inline uint64_t GetBucketByHamming(KademliaDistance const &dist)
{
  uint64_t ret{0};
  for (auto const &d : dist)
  {
    ret += platform::CountSetBits(d);
  }
  return ret;
}

inline uint64_t GetBucketByLogarithm(KademliaDistance const &dist)
{

  static uint64_t zeros[16] = {
      4,  // 0: 0b0000
      3,  // 1: 0b0001
      2,  // 2: 0b0010
      2,  // 3: 0b0011
      1,  // 4: 0b0100
      1,  // 5: 0b0101
      1,  // 6: 0b0110
      1,  // 7: 0b0111
      0,  // 8: 0b1000
      0,  // 9: 0b1001
      0,  // 10: 0b1010
      0,  // 11: 0b1011
      0,  // 12: 0b1100
      0,  // 13: 0b1101
      0,  // 14: 0b1110
      0   // 15: 0b1111
  };

  uint64_t    ret{8 * dist.size() * sizeof(uint8_t)};
  std::size_t i = dist.size();

  // TODO: this function can be improved by using intrinsics
  // but endianness needs to be considered.
  do
  {
    --i;
    auto top = zeros[(dist[i] >> 4) & 0xF];
    ret -= top;
    if (top == 4)
    {
      ret -= zeros[dist[i] & 0xF];
    }

  } while ((i != 0) && (dist[i] == 0));

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