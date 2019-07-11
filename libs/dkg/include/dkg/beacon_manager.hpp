#pragma once
#include "crypto/bls_base.hpp"
#include "crypto/bls_dkg.hpp"
#include "crypto/identity.hpp"
#include "crypto/ecdsa.hpp"

#include <unordered_map>

namespace fetch {
namespace crypto {

class BeaconManager
{
public:
  using VerificationVector = bls::dkg::VerificationVector;
  using ParticipantVector  = bls::dkg::ParticipantVector;
  using Signature          = bls::Signature;
  using Certificate        = std::shared_ptr<ECDSASigner>;  
  using PublicKey          = bls::PublicKey;

  using Contribution       = bls::dkg::Contribution;
  using ConstByteArray     = fetch::byte_array::ConstByteArray;

  BeaconManager() = default;
  BeaconManager(BeaconManager const &) = delete;
  BeaconManager& operator=(BeaconManager const &) = delete;

  // 1. Generate id
  void Reset(uint64_t cabinet_size, uint32_t threshold)
  {
    cabinet_size_ = cabinet_size;
    threshold_    = threshold;

    received_shares_.clear();
    participants_.clear();

    verification_vectors_.resize(cabinet_size);
    received_shares_.resize(cabinet_size);

    // Creating member id
    // TODO: Make loadable
    certificate_.reset( new ECDSASigner() );
    certificate_->GenerateKeys(); 
    auto id = bls::PrivateKeyByCSPRNG();
    id_.v = id.v;
  }

  // 2. Collect member ids
  PublicKey public_key()
  {
    return public_key_;
  }

  Identity identity()
  {
    return certificate_->identity();
  }

  bls::Id id() 
  {
    return id_;
  }

  void InsertMember(Identity identity, bls::Id id) 
  {
    identity_to_id_[identity] = participants_.size();
    participants_.push_back(id);
  }

  // 3. Generate shares
  void GenerateContribution()
  {
    contribution_ = bls::dkg::GenerateContribution(participants_, threshold_);
  }

  VerificationVector GetVerificationVector() const
  {
    return contribution_.verification;
  }

  bls::PrivateKey GetShare(Identity identity) const
  {
    auto it    = identity_to_id_.find(identity);

    if(it  == identity_to_id_.end())
    {
      throw std::runtime_error("Could not find identity in GetShare.");
    }

    uint64_t n = it->second;
    return contribution_.contributions[n];
  }

  // 4. Distribute shares
  bool AddShare(Identity from, bls::PrivateKey share, VerificationVector verification)
  {
    auto it     = identity_to_id_.find(from);
    assert(it  != identity_to_id_.end());

    if(it  == identity_to_id_.end())
    {
      throw std::runtime_error("Could not find identity in AddShare.");
    }

    uint64_t n = it->second;
    verification_vectors_[n] = verification;

    bool verified = bls::dkg::VerifyContributionShare(id_, share, verification);
    if(verified)
    {
      received_shares_[n] = share;
    }

    return verified;
  }

  // 5. Create key share
  void CreateKeyPair()
  {
    secret_key_share_ = bls::dkg::AccumulateContributionShares(received_shares_);

    // TODO: Can be optimised

    VerificationVector group_vectors = bls::dkg::AccumulateVerificationVectors(verification_vectors_);
    group_public_key_ = group_vectors[0];
    public_key_ = bls::PublicKeyFromPrivate(secret_key_share_);
  }


  // 6. Ready
  bool ready() const
  {
    return (cabinet_size_ == participants_.size())
        && (cabinet_size_ == verification_vectors_.size());        
  }

  struct SignedMessage
  {
    Signature signature;
    bls::PublicKey public_key;
  };

  void SetMessage(ConstByteArray next_message)
  {
    current_message_ = next_message;
  }

  // 7. Message
  SignedMessage Sign()
  {
    signature_buffer_.clear();
    signer_ids_.clear();

    SignedMessage smsg;
    smsg.signature   = bls::Sign(secret_key_share_, current_message_);
    smsg.public_key  = public_key_;


    if (!bls::Verify(smsg.signature, smsg.public_key, current_message_))
    {
      throw std::runtime_error("Failed to sign.");
    }

    return smsg;
  }

  ConstByteArray current_message() const
  {
    return current_message_;
  }


  //
  bool AddSignaturePart(Identity from, PublicKey public_key,  Signature signature)
  {
    auto it     = identity_to_id_.find(from);
    assert(it  != identity_to_id_.end());

    if(it  == identity_to_id_.end())
    {
      throw std::runtime_error("Could not find identity in AddSignaturePart.");
    }

    uint64_t n  = it->second;
    auto id = participants_[n];

    if (!bls::Verify(signature, public_key, current_message_))
    {
      throw std::runtime_error("Received signature is invalid.");
    }

    signer_ids_.push_back(id);
    signature_buffer_.push_back(signature);
    return true;
  }

  bool Verify()
  {
    auto signature = bls::RecoverSignature(signature_buffer_, signer_ids_);
    return bls::Verify(signature, group_public_key_, current_message_);
  }

  bool CanGenerateSignature()
  {
    // TODO(tfr): Yet to be added
    return true;
  }


private:
  uint64_t cabinet_size_{0};
  uint32_t threshold_{0};

  /// Member details
  /// @{
  std::unordered_map< Identity, uint64_t > identity_to_id_;
  /// @}


  /// Member identity and secrets
  /// @{
  Certificate      certificate_;
  bls::Id          id_;
  Contribution     contribution_;
  /// }

  /// Beacon keys
  /// @{
  bls::PrivateKey    secret_key_share_;  
  bls::PublicKey     group_public_key_;
  bls::PublicKey     public_key_;
  /// }

  /// Message signature management
  /// @{
  bls::SignatureList signature_buffer_;
  bls::IdList        signer_ids_;
  ConstByteArray     current_message_;
  /// }

  /// Details from other members
  /// @{
  bls::PrivateKeyList             received_shares_;
  ParticipantVector               participants_;
  std::vector< PublicKey >        public_keys_;
  std::vector<VerificationVector> verification_vectors_;
  /// }



};

}
}