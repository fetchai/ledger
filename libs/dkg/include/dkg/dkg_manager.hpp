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

#include "crypto/mcl_dkg.hpp"
#include "dkg/dkg_messages.hpp"

namespace fetch {
namespace dkg {

class DkgManager
{
public:
  using MuddleAddress    = byte_array::ConstByteArray;
  using Share            = DKGMessage::Share;
  using Coefficient      = DKGMessage::Coefficient;
  using ComplaintAnswer  = std::pair<MuddleAddress, std::pair<Share, Share>>;
  using ExposedShare     = std::pair<MuddleAddress, std::pair<Share, Share>>;
  using SharesExposedMap = std::unordered_map<MuddleAddress, std::pair<Share, Share>>;
  using Certificate      = crypto::Prover;
  using CertificatePtr   = std::shared_ptr<Certificate>;
  using Signature        = crypto::mcl::Signature;
  using PublicKey        = crypto::mcl::PublicKey;
  using PrivateKey       = crypto::mcl::PrivateKey;
  using CabinetIndex     = crypto::mcl::CabinetIndex;
  using MessagePayload   = crypto::mcl::MessagePayload;
  using Identity         = crypto::Identity;

  enum class AddResult
  {
    SUCCESS,
    NOT_MEMBER,
    SIGNATURE_ALREADY_ADDED,
    INVALID_SIGNATURE
  };

  struct SignedMessage
  {
    Signature signature;
    PublicKey public_key;
    Identity  identity;
  };

  DkgManager(CertificatePtr = nullptr);

  DkgManager(DkgManager const &) = delete;
  DkgManager &operator=(DkgManager const &) = delete;

  void SetCertificate(CertificatePtr certificate)
  {
    certificate_ = certificate;
  }

  void                     GenerateCoefficients();
  std::vector<Coefficient> GetCoefficients();
  std::pair<Share, Share>  GetOwnShares(MuddleAddress const &share_receiver);
  std::pair<Share, Share>  GetReceivedShares(MuddleAddress const &share_owner);
  void AddCoefficients(MuddleAddress const &from, std::vector<Coefficient> const &coefficients);
  void AddShares(MuddleAddress const &from, std::pair<Share, Share> const &shares);
  std::unordered_set<MuddleAddress> ComputeComplaints();
  bool VerifyComplaintAnswer(MuddleAddress const &from, ComplaintAnswer const &answer);
  void ComputeSecretShare();
  std::vector<Coefficient> GetQualCoefficients();
  void AddQualCoefficients(MuddleAddress const &from, std::vector<Coefficient> const &coefficients);
  SharesExposedMap ComputeQualComplaints();
  MuddleAddress    VerifyQualComplaint(MuddleAddress const &from, ComplaintAnswer const &answer);
  void             ComputePublicKeys();
  void             AddReconstructionShare(MuddleAddress const &address);
  void             AddReconstructionShare(MuddleAddress const &                  from,
                                          std::pair<MuddleAddress, Share> const &share);
  void             VerifyReconstructionShare(MuddleAddress const &from, ExposedShare const &share);
  bool             RunReconstruction();
  void             SetDkgOutput(bn::G2 &public_key, bn::Fr &secret_share,
                                std::vector<bn::G2> &public_key_shares, std::set<MuddleAddress> &qual);
  void             SetQual(std::set<MuddleAddress> qual);
  void             Reset(std::set<MuddleAddress> const &cabinet, uint32_t threshold);

  AddResult     AddSignaturePart(Identity const &from, PublicKey, Signature signature);
  bool          Verify();
  bool          Verify(Signature const &);
  Signature     GroupSignature() const;
  void          SetMessage(MessagePayload next_message);
  SignedMessage Sign();

  /// Property methods
  /// @{
  std::set<MuddleAddress> const &qual() const
  {
    return qual_;
  }

  uint32_t polynomial_degree() const
  {
    return polynomial_degree_;
  }
  CabinetIndex cabinet_index() const
  {
    return cabinet_index_;
  }
  CabinetIndex cabinet_index(MuddleAddress const &address) const
  {
    assert(identity_to_index_.find(address) != identity_to_index_.end());
    return identity_to_index_.at(address);
  }
  bool can_verify()
  {
    return signature_buffer_.size() >= polynomial_degree_ + 1;
  }
  ///}

private:
  static bn::G2 zeroG2_;   ///< Zero for public key type
  static bn::Fr zeroFr_;   ///< Zero for private key type
  static bn::G2 group_g_;  ///< Generator of group used in DKG
  static bn::G2 group_h_;  ///< Generator of subgroup used in DKG

  CertificatePtr certificate_;
  uint32_t       cabinet_size_;       ///< Size of committee
  uint32_t       polynomial_degree_;  ///< Degree of polynomial in DKG
  CabinetIndex   cabinet_index_;      ///< Index of our address in cabinet_

  /// Member details
  /// @{
  std::unordered_map<MuddleAddress, CabinetIndex> identity_to_index_;
  /// @}

  // What the DKG should return
  bn::Fr                  secret_share_;       ///< Share of group private key (x_i)
  bn::G2                  public_key_;         ///< Group public key (y)
  std::vector<bn::G2>     public_key_shares_;  ///< Public keys of cabinet generated by DKG (v_i)
  std::set<MuddleAddress> qual_;               ///< Set of qualified members

  // Temporary for DKG construction
  bn::Fr                           xprime_i;
  std::vector<bn::G2>              y_i;
  std::vector<std::vector<bn::Fr>> s_ij, sprime_ij;  ///< Secret shares
  std::vector<bn::Fr>              z_i;
  std::vector<std::vector<bn::G2>> C_ik;  ///< Verification vectors from cabinet members
  std::vector<std::vector<bn::G2>> A_ik;  ///< Qual verification vectors
  std::vector<std::vector<bn::G2>> g__s_ij;
  std::vector<bn::G2>              g__a_i;

  std::unordered_map<MuddleAddress, std::pair<std::set<CabinetIndex>, std::vector<bn::Fr>>>
      reconstruction_shares;  ///< Map from id of node_i in complaints to a pair <parties which
                              ///< exposed shares of node_i, the shares that were exposed>

  /// Message signature management
  /// @{
  std::unordered_set<MuddleAddress>           already_signed_;
  std::unordered_map<CabinetIndex, Signature> signature_buffer_;
  MessagePayload                              current_message_;
  Signature                                   group_signature_;
  /// }
};
}  // namespace dkg

namespace serializers {

template <typename D>
struct MapSerializer<dkg::DkgManager::SignedMessage, D>
{
public:
  using Type       = dkg::DkgManager::SignedMessage;
  using DriverType = D;

  static uint8_t const SIGNATURE  = 0;
  static uint8_t const PUBLIC_KEY = 1;
  static uint8_t const IDENTITY   = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &member)
  {
    auto map = map_constructor(3);

    map.Append(SIGNATURE, member.signature.getStr());
    map.Append(PUBLIC_KEY, member.public_key.getStr());
    map.Append(IDENTITY, member.identity);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &member)
  {
    std::string sig_str;
    std::string key_str;
    map.ExpectKeyGetValue(SIGNATURE, sig_str);
    map.ExpectKeyGetValue(PUBLIC_KEY, key_str);
    member.signature.setStr(sig_str);
    member.public_key.setStr(key_str);
    map.ExpectKeyGetValue(IDENTITY, member.identity);
  }
};
}  // namespace serializers
}  // namespace fetch
