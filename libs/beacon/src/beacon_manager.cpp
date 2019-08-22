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

namespace fetch {
namespace dkg {

void BeaconManager::Reset(uint64_t cabinet_size, uint32_t threshold)
{
  cabinet_size_ = cabinet_size;
  threshold_    = threshold;

  received_shares_.clear();
  participants_.clear();

  verification_vectors_.resize(cabinet_size);
  received_shares_.resize(cabinet_size);

  // Creating member id
  auto id = crypto::bls::PrivateKeyByCSPRNG();
  id_.v   = id.v;
}

bool BeaconManager::InsertMember(BeaconManager::Identity identity, BeaconManager::Id id)
{
  auto it = identity_to_index_.find(identity);
  if (it != identity_to_index_.end())
  {
    return false;
  }

  // Note that participant size increases every time that InsertMember is called.
  // This ensures that every unique identity is given a unqiue id.
  identity_to_index_[identity] = participants_.size();
  participants_.push_back(id);
  return true;
}

void BeaconManager::GenerateContribution()
{
  contribution_ = crypto::bls::dkg::GenerateContribution(participants_, threshold_);
}

BeaconManager::VerificationVector BeaconManager::GetVerificationVector() const
{
  return contribution_.verification;
}

BeaconManager::PrivateKey BeaconManager::GetShare(BeaconManager::Identity identity) const
{
  auto it = identity_to_index_.find(identity);

  if (it == identity_to_index_.end())
  {
    throw std::runtime_error("Could not find identity in GetShare.");
  }

  uint64_t n = it->second;
  return contribution_.contributions[n];
}

bool BeaconManager::AddShare(BeaconManager::Identity from, BeaconManager::PrivateKey share,
                             BeaconManager::VerificationVector verification)
{
  auto it = identity_to_index_.find(from);
  assert(it != identity_to_index_.end());

  if (it == identity_to_index_.end())
  {
    throw std::runtime_error("Could not find identity in AddShare.");
  }

  uint64_t n               = it->second;
  verification_vectors_[n] = verification;

  bool verified = crypto::bls::dkg::VerifyContributionShare(id_, share, verification);
  if (verified)
  {
    received_shares_[n] = share;
  }

  return verified;
}

void BeaconManager::CreateKeyPair()
{
  secret_key_share_ = crypto::bls::dkg::AccumulateContributionShares(received_shares_);

  CreateGroupPublicKey(verification_vectors_);
  public_key_ = crypto::bls::PublicKeyFromPrivate(secret_key_share_);
}

void BeaconManager::CreateGroupPublicKey(
    std::vector<VerificationVector> const &verification_vectors)
{
  // TODO(tfr): Can be optimised
  VerificationVector group_vectors =
      crypto::bls::dkg::AccumulateVerificationVectors(verification_vectors);
  group_public_key_ = group_vectors[0];
}

void BeaconManager::SetMessage(BeaconManager::ConstByteArray next_message)
{
  current_message_ = next_message;
  signature_buffer_.clear();
  signer_ids_.clear();
  already_signed_.clear();
}

BeaconManager::SignedMessage BeaconManager::Sign()
{
  SignedMessage smsg;
  smsg.signature  = crypto::bls::Sign(secret_key_share_, current_message_);
  smsg.public_key = public_key_;
  smsg.identity   = identity();

  if (!crypto::bls::Verify(smsg.signature, smsg.public_key, current_message_))
  {
    throw std::runtime_error("Failed to sign.");
  }

  return smsg;
}

BeaconManager::AddResult BeaconManager::AddSignaturePart(BeaconManager::Identity  from,
                                                         BeaconManager::PublicKey public_key,
                                                         BeaconManager::Signature signature)
{
  auto it = identity_to_index_.find(from);
  assert(it != identity_to_index_.end());

  if (it == identity_to_index_.end())
  {
    return AddResult::NOT_MEMBER;
  }

  if (already_signed_.find(from) != already_signed_.end())
  {
    return AddResult::SIGNATURE_ALREADY_ADDED;
  }

  uint64_t n  = it->second;
  auto     id = participants_[n];

  if (!crypto::bls::Verify(signature, public_key, current_message_))
  {
    return AddResult::INVALID_SIGNATURE;
  }

  signer_ids_.push_back(id);
  signature_buffer_.push_back(signature);
  already_signed_.insert(from);
  return AddResult::SUCCESS;
}

bool BeaconManager::Verify()
{
  group_signature_ = crypto::bls::RecoverSignature(signature_buffer_, signer_ids_);
  return Verify(group_signature_);
}

bool BeaconManager::Verify(Signature const &group_signature)
{
  return crypto::bls::Verify(group_signature, group_public_key_, current_message_);
}

}  // namespace dkg
}  // namespace fetch
