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
#include "crypto/mcl_dkg.hpp"

#include <mutex>

namespace fetch {
namespace ledger {
class NotarisationManager
{
public:
  using ConstByteArray      = byte_array::ConstByteArray;
  using MuddleAddress       = ConstByteArray;
  using PublicKey           = crypto::mcl::PublicKey;
  using PrivateKey          = crypto::mcl::PrivateKey;
  using Signature           = crypto::mcl::Signature;
  using Generator           = crypto::mcl::Generator;
  using MessagePayload      = crypto::mcl::MessagePayload;
  using AggregatePublicKey  = crypto::mcl::AggregatePublicKey;
  using AggregatePrivateKey = crypto::mcl::AggregatePrivateKey;
  using AggregateSignature  = crypto::mcl::AggregateSignature;

  NotarisationManager();

  /// Setup management
  /// @{
  PublicKey GenerateKeys();
  void      SetAeonDetails(uint64_t round_start, uint64_t round_end, uint32_t threshold,
                           std::map<MuddleAddress, PublicKey> const &cabinet_public_keys);
  /// @}

  /// Construction and verification of aggregate signatures
  /// @{
  Signature          Sign(MessagePayload const &message);
  bool               Verify(MessagePayload const &message, Signature const &signature,
                            MuddleAddress const &member);
  AggregateSignature ComputeAggregateSignature(
      std::unordered_map<MuddleAddress, Signature> const &cabinet_signatures);
  bool        VerifyAggregateSignature(MessagePayload const &    message,
                                       AggregateSignature const &aggregate_signature);
  static bool VerifyAggregateSignature(MessagePayload const &        message,
                                       AggregateSignature const &    aggregate_signature,
                                       std::vector<PublicKey> const &public_keys);
  /// @}

  /// Helper functions
  /// @{
  uint32_t                      Index(MuddleAddress const &member) const;
  bool                          CanSign() const;
  uint64_t                      round_start() const;
  uint64_t                      round_end() const;
  uint32_t                      threshold() const;
  std::set<MuddleAddress> const notarisation_members() const;
  /// @}

private:
  static Generator const &GetGenerator();

  // Aeon details
  uint64_t                                    round_start_{0};
  uint64_t                                    round_end_{0};
  uint32_t                                    threshold_{0};
  std::set<MuddleAddress>                     notarisation_members_{};
  std::unordered_map<MuddleAddress, uint32_t> identity_to_index_{};

  // Notarisation keys for this aeon
  AggregatePrivateKey             aggregate_private_key_;
  PublicKey                       public_key_;
  std::vector<AggregatePublicKey> cabinet_public_keys_{};
};
}  // namespace ledger
}  // namespace fetch
