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

#include "crypto/bls_base.hpp"
#include "crypto/bls_dkg.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/identity.hpp"

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

  using Signature  = crypto::bls::Signature;
  using PublicKey  = crypto::bls::PublicKey;
  using PrivateKey = crypto::bls::PrivateKey;
  using Id         = crypto::bls::Id;

  using ECDSASigner = crypto::ECDSASigner;
  using Certificate = std::shared_ptr<ECDSASigner>;
  using Identity    = crypto::Identity;

  using Contribution   = crypto::bls::dkg::Contribution;
  using ConstByteArray = fetch::byte_array::ConstByteArray;

  struct SignedMessage
  {
    Signature signature;
    PublicKey public_key;
  };

  BeaconManager()                      = default;
  BeaconManager(BeaconManager const &) = delete;
  BeaconManager &operator=(BeaconManager const &) = delete;

  /*
   * @brief resets the class back to a state where a new cabinet is set up.
   * @param cabinet_size is the size of the cabinet.
   * @param threshold is the threshold to be able to generate a signature.
   */
  void Reset(uint64_t cabinet_size, uint32_t threshold);

  /*
   * @brief adds a member to the current cabinet.
   * @param identity is the network identity of the node
   * @param id is the BLS identifier used in the algorithm.
   * @returns true if successful.
   */
  bool InsertMember(Identity identity, Id id);

  /*
   * @brief generates the shares and verification vectors.
   */
  void GenerateContribution();

  /*
   * @brief returns the verification vector.
   */
  VerificationVector GetVerificationVector() const;

  /*
   * @brief gets a share for given peer.
   * @param identity is the identity of the peer.
   */
  PrivateKey GetShare(Identity identity) const;

  /*
   * @brief adds a share from a peer to the internal share register
   * @param identity is the identity of the peer.
   * @param verification is the peers verification vector.
   */
  bool AddShare(Identity from, PrivateKey share, VerificationVector verification);

  /*
   * @brief creates the group key pair.
   */
  void CreateKeyPair();

  /*
   * @brief sets the next message to be signed.
   * @param next_message is the message to be signed.
   */
  void SetMessage(ConstByteArray next_message);

  /*
   * @brief signs the current message.
   */
  SignedMessage Sign();

  /*
   * @brief adds a signature share.
   * @param from is the identity of the sending node.
   * @param public_key is the public key of the peer.
   * @param signature is the signature part.
   */
  bool AddSignaturePart(Identity from, PublicKey public_key, Signature signature);

  /*
   * @brief verifies the group signature.
   */
  bool Verify();

  /// Property methods
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
  std::unordered_map<Identity, uint64_t> identity_to_index_;
  /// @}

  /// Member identity and secrets
  /// @{
  Certificate  certificate_;
  Id           id_;
  Contribution contribution_;
  /// }

  /// Beacon keys
  /// @{
  PrivateKey secret_key_share_;
  PublicKey  group_public_key_;
  PublicKey  public_key_;
  /// }

  /// Message signature management
  /// @{
  SignatureList  signature_buffer_;
  IdList         signer_ids_;
  ConstByteArray current_message_;
  /// }

  /// Details from other members
  /// @{
  PrivateKeyList                  received_shares_;
  ParticipantVector               participants_;
  std::vector<PublicKey>          public_keys_;
  std::vector<VerificationVector> verification_vectors_;
  /// }
};

}  // namespace dkg
}  // namespace fetch
