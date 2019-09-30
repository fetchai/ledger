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

#include "beacon/beacon_manager.hpp"
#include "ledger/chain/digest.hpp"

#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"

namespace fetch {
namespace beacon {

class BlockEntropyInterface
{
public:
  using Digest = byte_array::ConstByteArray;

  BlockEntropyInterface()          = default;
  virtual ~BlockEntropyInterface() = default;

  virtual Digest   EntropyAsSHA256() const = 0;
  virtual uint64_t EntropyAsU64() const    = 0;
};

struct BlockEntropy : public BlockEntropyInterface
{
  using MuddleAddress     = byte_array::ConstByteArray;
  using GroupPublicKey    = std::string;
  using MemberPublicKey   = byte_array::ConstByteArray;
  using MemberSignature   = byte_array::ConstByteArray;
  using Confirmations     = std::map<MemberPublicKey, MemberSignature>;
  using GroupSignature    = dkg::BeaconManager::Signature;
  using GroupSignatureStr = std::string;
  using Cabinet           = std::set<MuddleAddress>;

  BlockEntropy()
  {
    bn::initPairing();

    // Important this is cleared so the hash of it is consistent for the genesis block (?)
    group_signature = GroupSignature{};
    group_signature.clear();
  }

  Cabinet qualified;  // The members who succeeded DKG and are qualified to produce blocks (when new
                      // committee)
  GroupPublicKey group_public_key;  // The group public key (when new committee)
  uint64_t       block_number = 0;  // The block this is relevant to
  Digest digest;  // The hash of the above (when new committee) note, this could be implicit. Is not
                  // serialized.

  Confirmations confirmations;     // In the case of a new cabinet, personal signatures of the hash
                                   // from qual members
  GroupSignature group_signature;  // Signature of the previous entropy, used as the entropy

  Digest EntropyAsSHA256() const override
  {
    return crypto::Hash<crypto::SHA256>(group_signature.getStr());
  }

  // This will always be safe so long as the entropy function is properly sha256-ing
  uint64_t EntropyAsU64() const override
  {
    const Digest hash = EntropyAsSHA256();
    return *reinterpret_cast<uint64_t const *>(hash.pointer());
  }

  void HashSelf()
  {
    fetch::serializers::MsgPackSerializer serializer;
    serializer << qualified;
    serializer << group_public_key;
    serializer << block_number;
    digest = crypto::Hash<crypto::SHA256>(serializer.data());
  }

  bool IsAeonBeginning() const
  {
    return !qualified.empty();
  }
};

}  // namespace beacon

namespace serializers {

template <typename D>
struct MapSerializer<beacon::BlockEntropy, D>
{
public:
  using Type       = beacon::BlockEntropy;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &member)
  {
    auto map = map_constructor(5);

    map.Append(5, member.qualified);
    map.Append(4, member.group_public_key);
    map.Append(3, member.block_number);
    map.Append(2, member.confirmations);

    std::string set = member.group_signature.getStr();
    map.Append(1, set);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &member)
  {
    map.ExpectKeyGetValue(5, member.qualified);
    map.ExpectKeyGetValue(4, member.group_public_key);
    map.ExpectKeyGetValue(3, member.block_number);
    map.ExpectKeyGetValue(2, member.confirmations);

    std::string get;
    map.ExpectKeyGetValue(1, get);
    member.group_signature.setStr(get);
  }
};
}  // namespace serializers

}  // namespace fetch
