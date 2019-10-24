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
#include "beacon/block_entropy_interface.hpp"
#include "core/digest.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"

namespace fetch {
namespace beacon {

struct BlockEntropy : public BlockEntropyInterface
{
  using MuddleAddress         = byte_array::ConstByteArray;
  using GroupPublicKey        = byte_array::ConstByteArray;
  using MemberPublicKey       = byte_array::ConstByteArray;
  using MemberSignature       = byte_array::ConstByteArray;
  using Confirmations         = std::map<MemberPublicKey, MemberSignature>;
  using GroupSignature        = byte_array::ConstByteArray;
  using NotarisationKey       = byte_array::ConstByteArray;
  using ECDSASignature        = byte_array::ConstByteArray;
  using Cabinet               = std::set<MuddleAddress>;
  using SignedNotarisationKey = std::pair<NotarisationKey, ECDSASignature>;
  using CabinetMemberDetails  = std::map<MuddleAddress, SignedNotarisationKey>;
  using AggregateSignature    = std::pair<GroupSignature, std::vector<bool>>;

  BlockEntropy();
  BlockEntropy(BlockEntropy const &rhs);
  
  // When new committee, block contains muddle address of those who suceeded the DKG and
  // are qualified to produce blocks, and notarisation key (signed)
  Cabinet              qualified;
  CabinetMemberDetails member_details{};

  // The group public key (when new cabinet)
  GroupPublicKey group_public_key;

  // The block this is relevant to
  uint64_t block_number = 0;
  // The hash of the above (when new cabinet) note, this could be implicit. Is not
  // serialized.
  Digest digest;

  // In the case of a new cabinet, personal signatures of the hash
  // from qual members
  Confirmations confirmations;

  // Signature of the previous entropy, used as the entropy
  GroupSignature group_signature{};

  // Notarisation of block
  AggregateSignature block_notarisation;

  Digest EntropyAsSHA256() const override;

  // This will always be safe so long as the entropy function is properly sha256-ing
  uint64_t EntropyAsU64() const override;
  void     HashSelf();
  bool     IsAeonBeginning() const;
};

}  // namespace beacon

namespace serializers {

template <typename D>
struct MapSerializer<beacon::BlockEntropy, D>
{
public:
  using Type       = beacon::BlockEntropy;
  using DriverType = D;

  static uint8_t const QUALIFIED            = 1;
  static uint8_t const GROUP_PUBLIC_KEY     = 2;
  static uint8_t const BLOCK_NUMBER         = 3;
  static uint8_t const CONFIRMATIONS        = 4;
  static uint8_t const GROUP_SIGNATURE      = 5;
  static uint8_t const MEMBER_DETAILS       = 6;
  static uint8_t const NOTARISATION         = 7;
  static uint8_t const NOTARISATION_MEMBERS = 8;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &member)
  {
    auto map = map_constructor(8);

    map.Append(QUALIFIED, member.qualified);
    map.Append(GROUP_PUBLIC_KEY, member.group_public_key);
    map.Append(BLOCK_NUMBER, member.block_number);
    map.Append(CONFIRMATIONS, member.confirmations);
    map.Append(GROUP_SIGNATURE, member.group_signature);
    map.Append(MEMBER_DETAILS, member.member_details);
    map.Append(NOTARISATION, member.block_notarisation.first);
    // map.Append(NOTARISATION_MEMBERS, member.block_notarisation.second);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &member)
  {
    map.ExpectKeyGetValue(QUALIFIED, member.qualified);
    map.ExpectKeyGetValue(GROUP_PUBLIC_KEY, member.group_public_key);
    map.ExpectKeyGetValue(BLOCK_NUMBER, member.block_number);
    map.ExpectKeyGetValue(CONFIRMATIONS, member.confirmations);
    map.ExpectKeyGetValue(GROUP_SIGNATURE, member.group_signature);
    map.ExpectKeyGetValue(MEMBER_DETAILS, member.member_details);
    map.ExpectKeyGetValue(NOTARISATION, member.block_notarisation.first);
    // map.ExpectKeyGetValue(NOTARISATION_MEMBERS, member.block_notarisation.second);
  }
};
}  // namespace serializers

}  // namespace fetch
