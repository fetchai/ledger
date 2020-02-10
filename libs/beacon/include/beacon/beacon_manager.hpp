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

#include "beacon/dkg_output.hpp"
#include "core/serialisers/map_serialiser_boilerplate.hpp"
#include "crypto/mcl_dkg.hpp"
#include "dkg/dkg_messages.hpp"

namespace fetch {
namespace dkg {

class BeaconManager
{
public:
  using ConstByteArray   = byte_array::ConstByteArray;
  using MuddleAddress    = ConstByteArray;
  using DkgOutput        = beacon::DkgOutput;
  using Certificate      = crypto::Prover;
  using CertificatePtr   = std::shared_ptr<Certificate>;
  using Signature        = crypto::mcl::Signature;
  using PublicKey        = crypto::mcl::PublicKey;
  using PrivateKey       = crypto::mcl::PrivateKey;
  using Generator        = crypto::mcl::Generator;
  using CabinetIndex     = crypto::mcl::CabinetIndex;
  using MessagePayload   = crypto::mcl::MessagePayload;
  using Identity         = crypto::Identity;
  using Share            = DKGMessage::Share;
  using Coefficient      = DKGMessage::Coefficient;
  using ComplaintAnswer  = std::pair<MuddleAddress, std::pair<Share, Share>>;
  using ExposedShare     = std::pair<MuddleAddress, std::pair<Share, Share>>;
  using SharesExposedMap = std::unordered_map<MuddleAddress, std::pair<Share, Share>>;

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
    Identity  identity;
  };

  explicit BeaconManager(CertificatePtr certificate = CertificatePtr{});

  BeaconManager(BeaconManager const &) = delete;
  BeaconManager &operator=(BeaconManager const &) = delete;

  void SetCertificate(CertificatePtr certificate);

  void                     GenerateCoefficients();
  std::vector<Coefficient> GetCoefficients();
  std::pair<Share, Share>  GetOwnShares(MuddleAddress const &receiver);
  std::pair<Share, Share>  GetReceivedShares(MuddleAddress const &owner);
  void AddCoefficients(MuddleAddress const &from, std::vector<Coefficient> const &coefficients);
  void AddShares(MuddleAddress const &from, std::pair<Share, Share> const &shares);
  std::set<MuddleAddress> ComputeComplaints(std::set<MuddleAddress> const &coeff_received);
  bool VerifyComplaintAnswer(MuddleAddress const &from, ComplaintAnswer const &answer);
  void ComputeSecretShare();
  std::vector<Coefficient> GetQualCoefficients();
  void AddQualCoefficients(MuddleAddress const &from, std::vector<Coefficient> const &coefficients);
  SharesExposedMap ComputeQualComplaints(std::set<MuddleAddress> const &coeff_received);
  MuddleAddress    VerifyQualComplaint(MuddleAddress const &from, ComplaintAnswer const &answer);
  void             ComputePublicKeys();
  void             AddReconstructionShare(MuddleAddress const &address);
  void             VerifyReconstructionShare(MuddleAddress const &from, ExposedShare const &share);
  bool             RunReconstruction();
  DkgOutput        GetDkgOutput();
  void             SetDkgOutput(DkgOutput const &output);
  void             SetQual(std::set<MuddleAddress> qual);
  void             NewCabinet(std::set<MuddleAddress> const &cabinet, uint32_t threshold);
  void             Reset();

  AddResult     AddSignaturePart(Identity const &from, Signature const &signature);
  bool          Verify();
  bool          Verify(Signature const &signature);
  static bool   Verify(byte_array::ConstByteArray const &group_public_key,
                       MessagePayload const &message, byte_array::ConstByteArray const &signature);
  Signature     GroupSignature() const;
  void          SetMessage(MessagePayload next_message);
  SignedMessage Sign();

  /// Property methods
  /// @{
  bool                           InQual(MuddleAddress const &address) const;
  std::set<MuddleAddress> const &qual() const;
  uint32_t                       polynomial_degree() const;
  CabinetIndex                   cabinet_index() const;
  CabinetIndex                   cabinet_index(MuddleAddress const &address) const;
  bool                           can_verify();
  std::string                    group_public_key() const;
  ///}
  //

  template <typename D>
  struct MapSerialiser;

private:
  template <uint8_t KEY, class MemberVariable, MemberVariable MEMBER_VARIABLE>
  friend struct SerialisedStructField;

  // What the DKG should return
  PrivateKey              secret_share_;       ///< Share of group private key (x_i)
  PublicKey               public_key_;         ///< Group public key (y)
  std::vector<PublicKey>  public_key_shares_;  ///< Public keys of cabinet generated by DKG (v_i)
  std::set<MuddleAddress> qual_;               ///< Set of qualified members

private:
  static Generator const & GetGroupG();
  static Generator const & GetGroupH();
  static PrivateKey const &GetZeroFr();

  CertificatePtr certificate_;
  uint32_t       cabinet_size_;       ///< Size of cabinet
  uint32_t       polynomial_degree_;  ///< Degree of polynomial in DKG
  CabinetIndex   cabinet_index_;      ///< Index of our address in cabinet_

  /// Member details
  /// @{
  std::unordered_map<MuddleAddress, CabinetIndex> identity_to_index_;
  /// @}

  // Temporary for DKG construction
  PrivateKey                           xprime_i;
  std::vector<PublicKey>               y_i;
  std::vector<std::vector<PrivateKey>> s_ij, sprime_ij;  ///< Secret shares
  std::vector<std::vector<PublicKey>>  C_ik;  ///< Verification vectors from cabinet members
  std::vector<std::vector<PublicKey>>  A_ik;  ///< Qual verification vectors
  std::vector<std::vector<PublicKey>>  g__s_ij;
  std::vector<PublicKey>               g__a_i;

  std::unordered_map<MuddleAddress, std::pair<std::set<CabinetIndex>, std::vector<PrivateKey>>>
      reconstruction_shares;  ///< Map from id of node_i in complaints to a pair <parties which
  ///< exposed shares of node_i, the shares that were exposed>

  /// Message signature management
  /// @{
  std::unordered_set<MuddleAddress>           already_signed_;
  std::unordered_map<CabinetIndex, Signature> signature_buffer_;
  MessagePayload                              current_message_;
  Signature                                   group_signature_;
  /// }

  void AddReconstructionShare(MuddleAddress const &                  from,
                              std::pair<MuddleAddress, Share> const &share);
};

template <typename D>
struct BeaconManager::MapSerialiser
  : serialisers::MapSerialiserBoilerplate<
        BeaconManager, D, serialiseD_STRUCT_FIELD(1, BeaconManager::secret_share_),
        serialiseD_STRUCT_FIELD(2, BeaconManager::public_key_),
        serialiseD_STRUCT_FIELD(3, BeaconManager::public_key_shares_),
        serialiseD_STRUCT_FIELD(4, BeaconManager::qual_),
        serialiseD_STRUCT_FIELD(5, BeaconManager::identity_to_index_),
        serialiseD_STRUCT_FIELD(6, BeaconManager::polynomial_degree_),
        serialiseD_STRUCT_FIELD(7, BeaconManager::cabinet_size_),
        serialiseD_STRUCT_FIELD(8, BeaconManager::cabinet_index_)>
{
};

}  // namespace dkg

namespace serialisers {

template <typename D>
struct MapSerialiser<dkg::BeaconManager, D> : dkg::BeaconManager::MapSerialiser<D>
{
};

template <typename D>
struct MapSerialiser<dkg::BeaconManager::SignedMessage, D>
  : MapSerialiserBoilerplate<
        dkg::BeaconManager::SignedMessage, D,
        serialiseD_STRUCT_FIELD(0, dkg::BeaconManager::SignedMessage::signature),
        serialiseD_STRUCT_FIELD(1, dkg::BeaconManager::SignedMessage::identity)>
{
};

}  // namespace serialisers
}  // namespace fetch
