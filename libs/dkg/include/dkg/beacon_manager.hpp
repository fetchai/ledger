#pragma once
#include "crypto/bls_base.hpp"
#include "crypto/bls_dkg.hpp"
#include "crypto/identity.hpp"
#include "crypto/ecdsa.hpp"

#include <unordered_map>

namespace fetch {
namespace dkg {

class BeaconManager
{
public:
  using VerificationVector = crypto::bls::dkg::VerificationVector;
  using ParticipantVector  = crypto::bls::dkg::ParticipantVector;
  using IdList             = crypto::bls::IdList;
  using SignatureList      = crypto::bls::SignatureList;
  using PrivateKeyList     = crypto::bls::PrivateKeyList;

  using Signature          = crypto::bls::Signature;
  using PublicKey          = crypto::bls::PublicKey;
  using PrivateKey         = crypto::bls::PrivateKey;  
  using Id                 = crypto::bls::Id;

  using ECDSASigner        = crypto::ECDSASigner;
  using Certificate        = std::shared_ptr<ECDSASigner>;  
  using Identity           = crypto::Identity;

  using Contribution       = crypto::bls::dkg::Contribution;
  using ConstByteArray     = fetch::byte_array::ConstByteArray;

  struct SignedMessage
  {
    Signature signature;
    PublicKey public_key;
  };

  BeaconManager() = default;
  BeaconManager(BeaconManager const &) = delete;
  BeaconManager& operator=(BeaconManager const &) = delete;

  void Reset(uint64_t cabinet_size, uint32_t threshold);
  void InsertMember(Identity identity, Id id);
  void GenerateContribution();
  VerificationVector GetVerificationVector() const;
  PrivateKey GetShare(Identity identity) const;
  bool AddShare(Identity from, PrivateKey share, VerificationVector verification);
  void CreateKeyPair();
  void SetMessage(ConstByteArray next_message);
  SignedMessage Sign();
  bool AddSignaturePart(Identity from, PublicKey public_key,  Signature signature);
  bool Verify();


  /// @{
  ConstByteArray current_message() const
  {
    return current_message_;
  }

  PublicKey public_key()
  {
    return public_key_;
  }

  Identity identity()
  {
    return certificate_->identity();
  }

  Id id() 
  {
    return id_;
  }
  /// }
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
  Id               id_;
  Contribution     contribution_;
  /// }

  /// Beacon keys
  /// @{
  PrivateKey    secret_key_share_;  
  PublicKey     group_public_key_;
  PublicKey     public_key_;
  /// }

  /// Message signature management
  /// @{
  SignatureList   signature_buffer_;
  IdList          signer_ids_;
  ConstByteArray  current_message_;
  /// }

  /// Details from other members
  /// @{
  PrivateKeyList                  received_shares_;
  ParticipantVector               participants_;
  std::vector< PublicKey >        public_keys_;
  std::vector<VerificationVector> verification_vectors_;
  /// }



};

}
}