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

  explicit DkgManager(MuddleAddress const &address);

  void                     GenerateCoefficients();
  std::vector<Coefficient> GetCoefficients();
  std::pair<Share, Share>  GetOwnShares(MuddleAddress const &share_receiver);
  std::pair<Share, Share>  GetReceivedShares(MuddleAddress const &share_owner);
  bool AddCoefficients(MuddleAddress const &from, std::vector<Coefficient> const &coefficients);
  bool AddShares(MuddleAddress const &from, std::pair<Share, Share> const &shares);
  std::unordered_set<MuddleAddress> ComputeComplaints();
  bool VerifyComplaintAnswer(MuddleAddress const &from, ComplaintAnswer const &answer);
  void ComputeSecretShare(std::set<MuddleAddress> const &qual);
  std::vector<Coefficient> GetQualCoefficients();
  bool AddQualCoefficients(MuddleAddress const &from, std::vector<Coefficient> const &coefficients);
  SharesExposedMap ComputeQualComplaints(std::set<MuddleAddress> const &qual);
  MuddleAddress    VerifyQualComplaint(MuddleAddress const &from, ComplaintAnswer const &answer);
  void             ComputePublicKeys(std::set<MuddleAddress> const &qual);
  void             AddReconstructionShare(MuddleAddress const &address);
  bool             CheckDuplicateReconstructionShare(MuddleAddress const &from,
                                                     MuddleAddress const &share_owner);
  void             AddReconstructionShare(MuddleAddress const &                  from,
                                          std::pair<MuddleAddress, Share> const &share);
  void             VerifyReconstructionShare(MuddleAddress const &from, ExposedShare const &share);
  bool             RunReconstruction();
  void             Reset(std::set<MuddleAddress> const &cabinet, uint32_t threshold);
  void             SetDkgOutput(bn::G2 &public_key, bn::Fr &secret_share,
                                std::vector<bn::G2> &public_key_shares);

  uint32_t polynomial_degree() const
  {
    return polynomial_degree_;
  }
  uint32_t cabinet_index() const
  {
    return cabinet_index_;
  }
  uint32_t cabinet_index(MuddleAddress const &address) const
  {
    assert(identity_to_index_.find(address) != identity_to_index_.end());
    return identity_to_index_.at(address);
  }

private:
  static bn::G2 zeroG2_;   ///< Zero for public key type
  static bn::Fr zeroFr_;   ///< Zero for private key type
  static bn::G2 group_g_;  ///< Generator of group used in DKG
  static bn::G2 group_h_;  ///< Generator of subgroup used in DKG

  MuddleAddress address_;
  uint32_t      cabinet_size_;       ///< Size of committee
  uint32_t      polynomial_degree_;  ///< Degree of polynomial in DKG
  uint32_t      cabinet_index_;      ///< Index of our address in cabinet_

  /// Member details
  /// @{
  std::unordered_map<MuddleAddress, uint32_t> identity_to_index_;
  /// @}

  // What the DKG should return
  bn::Fr              secret_share_;       ///< Share of group private key (x_i)
  bn::G2              public_key_;         ///< Group public key (y)
  std::vector<bn::G2> public_key_shares_;  ///< Public keys of cabinet generated by DKG (v_i)

  // Temporary for DKG construction
  bn::Fr                           xprime_i;
  std::vector<bn::G2>              y_i;
  std::vector<std::vector<bn::Fr>> s_ij, sprime_ij;
  std::vector<bn::Fr>              z_i;
  std::vector<std::vector<bn::G2>> C_ik;
  std::vector<std::vector<bn::G2>> A_ik;
  std::vector<std::vector<bn::G2>> g__s_ij;
  std::vector<bn::G2>              g__a_i;

  std::unordered_map<MuddleAddress, std::pair<std::set<uint32_t>, std::vector<bn::Fr>>>
      reconstruction_shares;  ///< Map from id of node_i in complaints to a pair <parties which
                              ///< exposed shares of node_i, the shares that were exposed>
};
}  // namespace dkg
}  // namespace fetch
