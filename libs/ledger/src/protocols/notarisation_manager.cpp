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

#include "ledger/protocols/notarisation_manager.hpp"

namespace fetch {
namespace ledger {
NotarisationManager::Generator NotarisationManager::generator_;

NotarisationManager::NotarisationManager()
{
  static std::once_flag flag;

  std::call_once(flag, []() {
    fetch::crypto::mcl::details::MCLInitialiser();
    generator_.clear();
    crypto::mcl::SetGenerator(generator_);
  });

  private_key_.clear();
  public_key_.clear();
}

NotarisationManager::Signature NotarisationManager::Sign(MessagePayload const &message)
{
  return crypto::mcl::SignShare(message, private_key_);
}

bool NotarisationManager::Verify(MessagePayload const &message, Signature const &signature,
                                 MuddleAddress const &member)
{
  uint32_t member_index = identity_to_index_[member];
  return crypto::mcl::VerifySign(cabinet_public_keys_[member_index], message, signature,
                                 generator_);
}

std::pair<NotarisationManager::Signature, std::vector<bool>>
NotarisationManager::ComputeAggregateSignature(
    std::unordered_map<MuddleAddress, Signature> const &cabinet_signatures)
{
  std::unordered_map<uint32_t, Signature> signatures;
  for (auto const &share : cabinet_signatures)
  {
    signatures.insert({identity_to_index_[share.first], share.second});
  }
  return crypto::mcl::ComputeAggregateSignature(signatures, cabinet_public_keys_);
}

bool NotarisationManager::VerifyAggregateSignature(
    MessagePayload const &                         message,
    std::pair<Signature, std::vector<bool>> const &aggregate_signature)
{
  return crypto::mcl::VerifyAggregateSignature(message, aggregate_signature, cabinet_public_keys_,
                                               generator_);
}

NotarisationManager::PublicKey NotarisationManager::GenerateKeys()
{
  if (private_key_.isZero())
  {
    auto keys    = crypto::mcl::GenerateKeyPair(generator_);
    private_key_ = keys.first;
    public_key_  = keys.second;
    return keys.second;
  }
  else
  {
    return public_key_;
  }
}

void NotarisationManager::SetAeonDetails(
    uint64_t round_start, uint64_t round_end, uint32_t threshold,
    std::map<MuddleAddress, PublicKey> const &cabinet_public_keys)
{
  round_start_ = round_start;
  round_end_   = round_end;
  threshold_   = threshold;

  uint32_t index = 0;
  for (auto const &member : cabinet_public_keys)
  {
    notarisation_members_.insert(member.first);
    identity_to_index_.insert({member.first, index});
    ++index;
  }

  crypto::mcl::Init(cabinet_public_keys_, static_cast<uint32_t>(identity_to_index_.size()));
  for (auto const &key : cabinet_public_keys)
  {
    cabinet_public_keys_[identity_to_index_[key.first]] = key.second;
  }
}

uint32_t NotarisationManager::Index(MuddleAddress const &member) const
{
  assert(identity_to_index_.find(member) != identity_to_index_.end());
  return identity_to_index_.at(member);
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
std::set<NotarisationManager::MuddleAddress> const &NotarisationManager::notarisation_members()
    const
{
  return notarisation_members_;
}
}  // namespace ledger
}  // namespace fetch
