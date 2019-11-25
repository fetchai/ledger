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

#include "beacon/notarisation_manager.hpp"
#include "core/synchronisation/protected.hpp"

namespace fetch {
namespace ledger {

Protected<std::shared_ptr<NotarisationManager::Generator>> generator_;

NotarisationManager::NotarisationManager()
{
  generator_.ApplyVoid([](std::shared_ptr<Generator> generator) {
    if (!generator)
    {
      generator = std::make_unique<Generator>();
      SetGenerator(*generator);
    }
  });
}

NotarisationManager::Signature NotarisationManager::Sign(MessagePayload const &message)
{
  return crypto::mcl::AggregateSign(message, aggregate_private_key_);
}

bool NotarisationManager::Verify(MessagePayload const &message, Signature const &signature,
                                 MuddleAddress const &member)
{
  uint32_t member_index = identity_to_index_[member];
  return crypto::mcl::VerifySign(cabinet_public_keys_[member_index].aggregate_public_key, message,
                                 signature, GetGenerator());
}

NotarisationManager::AggregateSignature NotarisationManager::ComputeAggregateSignature(
    std::unordered_map<MuddleAddress, Signature> const &cabinet_signatures)
{
  std::unordered_map<uint32_t, Signature> signatures;
  for (auto const &share : cabinet_signatures)
  {
    signatures.insert({identity_to_index_[share.first], share.second});
  }
  return crypto::mcl::ComputeAggregateSignature(signatures,
                                                static_cast<uint32_t>(identity_to_index_.size()));
}

bool NotarisationManager::VerifyAggregateSignature(MessagePayload const &    message,
                                                   AggregateSignature const &aggregate_signature)
{
  if (aggregate_signature.second.size() != cabinet_public_keys_.size())
  {
    return false;
  }
  PublicKey aggregate_public_key =
      crypto::mcl::ComputeAggregatePublicKey(aggregate_signature.second, cabinet_public_keys_);
  return crypto::mcl::VerifySign(aggregate_public_key, message, aggregate_signature.first,
                                 GetGenerator());
}

bool NotarisationManager::VerifyAggregateSignature(MessagePayload const &    message,
                                                   AggregateSignature const &aggregate_signature,
                                                   std::vector<PublicKey> const &public_keys)
{
  if (aggregate_signature.second.size() != public_keys.size())
  {
    return false;
  }
  PublicKey aggregate_public_key =
      crypto::mcl::ComputeAggregatePublicKey(aggregate_signature.second, public_keys);
  return crypto::mcl::VerifySign(aggregate_public_key, message, aggregate_signature.first,
                                 GetGenerator());
}

NotarisationManager::PublicKey NotarisationManager::GenerateKeys()
{
  if (aggregate_private_key_.private_key.isZero())
  {
    auto keys                          = crypto::mcl::GenerateKeyPair(GetGenerator());
    aggregate_private_key_.private_key = keys.first;
    public_key_                        = keys.second;
    return keys.second;
  }
  return public_key_;
}

void NotarisationManager::SetAeonDetails(
    uint64_t round_start, uint64_t round_end, uint32_t threshold,
    std::map<MuddleAddress, PublicKey> const &cabinet_public_keys)
{
  round_start_ = round_start;
  round_end_   = round_end;
  threshold_   = threshold;

  uint32_t               index = 0;
  std::vector<PublicKey> temp_keys;
  for (auto const &member : cabinet_public_keys)
  {
    notarisation_members_.insert(member.first);
    identity_to_index_.insert({member.first, index});
    temp_keys.push_back(member.second);
    ++index;
  }

  cabinet_public_keys_.resize(identity_to_index_.size());
  // Compute modified public keys
  for (auto const &member : cabinet_public_keys)
  {
    PrivateKey aggregate_coefficient =
        crypto::mcl::SignatureAggregationCoefficient(member.second, temp_keys);
    cabinet_public_keys_[identity_to_index_[member.first]] =
        AggregatePublicKey{member.second, aggregate_coefficient};
    // Set own aggregate coefficient for signing
    if (member.second == public_key_)
    {
      aggregate_private_key_.coefficient = aggregate_coefficient;
    }
  }

  // Coefficient should not be 0 if private key has been set
  assert(CanSign() == !aggregate_private_key_.coefficient.isZero());
}

uint32_t NotarisationManager::Index(MuddleAddress const &member) const
{
  assert(identity_to_index_.find(member) != identity_to_index_.end());
  return identity_to_index_.at(member);
}

bool NotarisationManager::CanSign() const
{
  return !aggregate_private_key_.private_key.isZero();
}

uint64_t NotarisationManager::round_start() const
{
  return round_start_;
}
uint64_t NotarisationManager::round_end() const
{
  return round_end_;
}
uint32_t NotarisationManager::threshold() const
{
  return threshold_;
}
std::set<NotarisationManager::MuddleAddress> const NotarisationManager::notarisation_members() const
{
  return notarisation_members_;
}

NotarisationManager::Generator const &NotarisationManager::GetGenerator()
{
  return *generator_.Apply([](std::shared_ptr<Generator> &generator) {
    if (!generator)
    {
      generator = std::make_unique<Generator>();
      SetGenerator(*generator);
    }
    return generator;
  });
}

}  // namespace ledger
}  // namespace fetch
