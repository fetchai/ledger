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

#include "beacon/beacon_manager.hpp"
#include "beacon/block_entropy_interface.hpp"
#include "beacon/notarisation_manager.hpp"
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
  using Confirmations         = std::map<uint16_t, MemberSignature>;
  using GroupSignature        = byte_array::ConstByteArray;
  using ECDSASignature        = byte_array::ConstByteArray;
  using NotarisationKey       = ledger::NotarisationManager::PublicKey;
  using AggregateSignature    = ledger::NotarisationManager::AggregateSignature;
  using Cabinet               = std::set<MuddleAddress>;
  using SignedNotarisationKey = std::pair<NotarisationKey, ECDSASignature>;
  using AeonNotarisationKeys  = std::map<MuddleAddress, SignedNotarisationKey>;

  BlockEntropy() = default;

  // When new committee, block contains muddle address of those who succeeded the DKG and
  // are qualified to produce blocks, and notarisation key (signed)
  Cabinet              qualified;
  AeonNotarisationKeys aeon_notarisation_keys{};

  // The group public key (when new cabinet)
  GroupPublicKey group_public_key;

  // The block this is relevant to
  uint64_t block_number = 0;
  // The hash of the above (when new cabinet) note, this is populated on deser.
  Digest digest;

  // In the case of a new cabinet, personal signatures of the hash
  // from qual members
  Confirmations confirmations;

  // Signature of the previous entropy, used as the entropy
  GroupSignature group_signature{};

  // Notarisation of block
  AggregateSignature block_notarisation;

  void   SelectCopy(BlockEntropy const &rhs);
  Digest EntropyAsSHA256() const override;

  // This will always be safe so long as the entropy function is properly sha256-ing
  uint64_t EntropyAsU64() const override;
  void     HashSelf();
  bool     IsAeonBeginning() const;

  // Helper function - convert to the index used in the confirmations
  uint16_t ToQualIndex(MuddleAddress const &member) const;
};

}  // namespace beacon

namespace serializers {

struct BlockEntropyNotarisationAndHashSelf
{
  static constexpr std::size_t LogicalSize() noexcept
  {
    return 2;
  }

  static uint8_t const NOTARISATION         = 7;
  static uint8_t const NOTARISATION_MEMBERS = 8;

  template <class Map>
  static constexpr void Serialize(Map &map, beacon::BlockEntropy const &member) noexcept
  {
    map.Append(NOTARISATION, member.block_notarisation.first);
    map.Append(NOTARISATION_MEMBERS, member.block_notarisation.second);
  }

  template <class Map>
  static constexpr void Deserialize(Map &map, beacon::BlockEntropy &member)
  {
    map.ExpectKeyGetValue(NOTARISATION, member.block_notarisation.first);
    map.ExpectKeyGetValue(NOTARISATION_MEMBERS, member.block_notarisation.second);

    if (!member.confirmations.empty())
    {
      member.HashSelf();
    }
  }
};

// clang-format off
template <typename D>
struct MapSerializer<beacon::BlockEntropy, D>
  : MapSerializerBoilerplate<beacon::BlockEntropy, D,
        EXPECTED_KEY_MEMBER(1, beacon::BlockEntropy::qualified),
        EXPECTED_KEY_MEMBER(2, beacon::BlockEntropy::group_public_key),
        EXPECTED_KEY_MEMBER(3, beacon::BlockEntropy::block_number),
        EXPECTED_KEY_MEMBER(4, beacon::BlockEntropy::confirmations),
        EXPECTED_KEY_MEMBER(5, beacon::BlockEntropy::group_signature),
        EXPECTED_KEY_MEMBER(6, beacon::BlockEntropy::aeon_notarisation_keys),
        BlockEntropyNotarisationAndHashSelf>
{
};
// clang-format on

}  // namespace serializers
}  // namespace fetch
