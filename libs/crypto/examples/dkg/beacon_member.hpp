#pragma once
#include "crypto/bls_base.hpp"
#include "crypto/bls_dkg.hpp"
#include "crypto/identity.hpp"
#include "crypto/ecdsa.hpp"

#include <unordered_map>

namespace fetch {
namespace crypto {

class BeaconMember
{
public:
  using VerificationVector = bls::dkg::VerificationVector;
  using ParticipantVector  = bls::dkg::ParticipantVector;
  using Certificate        = ECDSASigner;
  using Contribution       = bls::dkg::Contribution;
  using ConstByteArray     = fetch::byte_array::ConstByteArray;

  BeaconMember() = default;

  // 1. Generate id
  void Reset(uint64_t cabinet_size, uint32_t threshold)
  {
    cabinet_size_ = cabinet_size;
    threshold_    = threshold;
    member_count_ = 0;

    received_shares_.clear();
    participants_.clear();

    // Creating member id
    // TODO: Make loadable
    certificate_.GenerateKeys(); 
    auto id = bls::PrivateKeyByCSPRNG();
    id_.v = id.v;
  }

  // 2. Collect member ids
  Identity GetIdentity()
  {
    return certificate_.identity();
  }

  bls::Id id() 
  {
    return id_;
  }

  void InsertMember(Identity identity, bls::Id id) 
  {
    identity_to_id_[identity] = id;
    participants_.push_back(id);
  }

  // 3. Generate shares
  void GenerateContribution()
  {
    contribution_ = bls::dkg::GenerateContribution(participants_, threshold_);
  }

  // 4. Distribute shares
  bool AddShare(Identity from, bls::PrivateKey share, VerificationVector verification)
  {
    auto id = identity_to_id_[from];

    bool verified = bls::dkg::VerifyContributionShare(id, share, verification);
    if(verified)
    {
      received_shares_.push_back(share);
    }

    return verified;
  }

  // 5. Create key share
  void CreateKeyPair()
  {
    secret_key_share_ = bls::dkg::AccumulateContributionShares(received_shares_);

    // TODO: Can be optimised
    VerificationVector group_vectors = bls::dkg::AccumulateVerificationVectors(verification_vectors_);
    shared_public_key_ = group_vectors[0];
  }

  // 6. Ready
  bool ready() const
  {
    return (cabinet_size_ == participants_.size())
        && (cabinet_size_ == verification_vectors_.size());        
  }
private:
  uint64_t cabinet_size_{0};
  uint32_t threshold_{0};

  /// Member details
  /// @{
  std::unordered_map< Identity, bls::Id > identity_to_id_;
  uint64_t member_count_{0};
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
  bls::PublicKey     shared_public_key_;
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
  std::vector<VerificationVector> verification_vectors_;
  /// }



};

}
}