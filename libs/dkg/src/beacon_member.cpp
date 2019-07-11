#include "dkg/beacon_manager.hpp"

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
  // TODO(tfr): Make loadable
  certificate_.reset( new ECDSASigner() );
  certificate_->GenerateKeys(); 
  auto id = crypto::bls::PrivateKeyByCSPRNG();
  id_.v = id.v;
}

void BeaconManager::InsertMember(BeaconManager::Identity identity, BeaconManager::Id id) 
{
  identity_to_id_[identity] = participants_.size();
  participants_.push_back(id);
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
  auto it    = identity_to_id_.find(identity);

  if(it  == identity_to_id_.end())
  {
    throw std::runtime_error("Could not find identity in GetShare.");
  }

  uint64_t n = it->second;
  return contribution_.contributions[n];
}

bool BeaconManager::AddShare(BeaconManager::Identity from, BeaconManager::PrivateKey share, BeaconManager::VerificationVector verification)
{
  auto it     = identity_to_id_.find(from);
  assert(it  != identity_to_id_.end());

  if(it  == identity_to_id_.end())
  {
    throw std::runtime_error("Could not find identity in AddShare.");
  }

  uint64_t n = it->second;
  verification_vectors_[n] = verification;

  bool verified = crypto::bls::dkg::VerifyContributionShare(id_, share, verification);
  if(verified)
  {
    received_shares_[n] = share;
  }

  return verified;
}

void BeaconManager::CreateKeyPair()
{
  secret_key_share_ = crypto::bls::dkg::AccumulateContributionShares(received_shares_);

  // TODO: Can be optimised

  VerificationVector group_vectors = crypto::bls::dkg::AccumulateVerificationVectors(verification_vectors_);
  group_public_key_ = group_vectors[0];
  public_key_ = crypto::bls::PublicKeyFromPrivate(secret_key_share_);
}

void BeaconManager::SetMessage(BeaconManager::ConstByteArray next_message)
{
  current_message_ = next_message;
}

BeaconManager::SignedMessage BeaconManager::Sign()
{
  signature_buffer_.clear();
  signer_ids_.clear();

  SignedMessage smsg;
  smsg.signature   = crypto::bls::Sign(secret_key_share_, current_message_);
  smsg.public_key  = public_key_;


  if (!crypto::bls::Verify(smsg.signature, smsg.public_key, current_message_))
  {
    throw std::runtime_error("Failed to sign.");
  }

  return smsg;
}

bool BeaconManager::AddSignaturePart(BeaconManager::Identity from, BeaconManager::PublicKey public_key, BeaconManager::Signature signature)
{
  auto it     = identity_to_id_.find(from);
  assert(it  != identity_to_id_.end());

  if(it  == identity_to_id_.end())
  {
    throw std::runtime_error("Could not find identity in AddSignaturePart.");
  }

  uint64_t n  = it->second;
  auto id = participants_[n];

  if (!crypto::bls::Verify(signature, public_key, current_message_))
  {
    throw std::runtime_error("Received signature is invalid.");
  }

  signer_ids_.push_back(id);
  signature_buffer_.push_back(signature);
  return true;
}

bool BeaconManager::Verify()
{
  auto signature = crypto::bls::RecoverSignature(signature_buffer_, signer_ids_);
  return crypto::bls::Verify(signature, group_public_key_, current_message_);
}


}
}