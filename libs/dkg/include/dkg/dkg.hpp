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
#include "dkg/dkg_complaints_manager.hpp"
#include "dkg/dkg_messages.hpp"
#include "network/muddle/rpc/client.hpp"

#include <atomic>
#include <mutex>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace muddle {

class MuddleEndpoint;

}  // namespace muddle

namespace dkg {

/**
 * //TODO(jmw): DKG protocol
 */
class DistributedKeyGeneration
{
  using MuddleAddress    = byte_array::ConstByteArray;
  using CabinetMembers   = std::set<MuddleAddress>;
  using Endpoint         = muddle::MuddleEndpoint;
  using MsgShare         = std::string;
  using SharesExposedMap = std::unordered_map<MuddleAddress, std::pair<MsgShare, MsgShare>>;

  enum class State : uint8_t
  {
    INITIAL,
    WAITING_FOR_SHARE,
    WAITING_FOR_COMPLAINTS,
    WAITING_FOR_COMPLAINT_ANSWERS,
    WAITING_FOR_QUAL_SHARES,
    WAITING_FOR_QUAL_COMPLAINTS,
    WAITING_FOR_RECONSTRUCTION_SHARES
  };
  static bn::G2 zeroG2_;   ///< Zero for public key type
  static bn::Fr zeroFr_;   ///< Zero for private key type
  static bn::G2 group_g_;  ///< Generator of group used in DKG
  static bn::G2 group_h_;  ///< Generator of subgroup used in DKG

  CabinetMembers const &cabinet_;    ///< Muddle addresses of cabinet members
  uint32_t const &      threshold_;  ///< Number of cooperating members required to generate keys
  MuddleAddress         address_;    ///< Our muddle address
  uint32_t              cabinet_index_;  ///< Index of our address in cabinet_
  // DkgService &       dkg_service_;
  std::function<void(DKGEnvelop const &)> broadcast_callback_;
  std::function<void(MuddleAddress const &, std::pair<std::string, std::string> const &)>
                     rpc_callback_;
  std::atomic<State> state_{State::INITIAL};
  std::mutex         mutex_;

  // What the DKG should return
  std::atomic<bool>       finished_{false};    ///< Marks whether the DKG has completed
  bn::Fr                  secret_share_;       ///< Share of group private key (x_i)
  bn::G2                  public_key_;         ///< Group public key (y)
  std::vector<bn::G2>     public_key_shares_;  ///< Public keys of cabinet generated by DKG (v_i)
  std::set<MuddleAddress> qual_;               ///< Set of cabinet members who completed DKG

  // Temporary for DKG construction
  bn::Fr                           xprime_i;
  std::vector<bn::G2>              y_i;
  std::vector<std::vector<bn::Fr>> s_ij, sprime_ij;
  std::vector<bn::Fr>              z_i;
  std::vector<std::vector<bn::G2>> C_ik;
  std::vector<std::vector<bn::G2>> A_ik;
  std::vector<std::vector<bn::G2>> g__s_ij;
  std::vector<bn::G2>              g__a_i;

  // Managing complaints
  ComplaintsManager       complaints_manager_;
  ComplaintsAnswerManager complaints_answer_manager_;
  QualComplaintsManager   qual_complaints_manager_;
  std::atomic<bool>       received_all_coef_and_shares_{false};
  std::atomic<bool>       received_all_complaints_{false};
  std::atomic<bool>       received_all_complaints_answer_{false};
  std::atomic<bool>       received_all_qual_shares_{false};
  std::atomic<bool>       received_all_qual_complaints_{false};
  std::atomic<bool>       received_all_reconstruction_shares_{false};

  // Counters for types of messages received
  std::atomic<uint32_t> shares_received_{0};
  std::atomic<uint32_t> C_ik_received_{0};
  std::atomic<uint32_t> A_ik_received_{0};
  std::atomic<uint32_t> reconstruction_shares_received_{0};

  std::unordered_map<MuddleAddress, std::pair<std::vector<uint32_t>, std::vector<bn::Fr>>>
      reconstruction_shares;  ///< Map from id of node_i in complaints to a pair <parties which
                              ///< exposed shares of node_i, the shares that were exposed>

  /// @name Methods to send messages
  /// @{
  void SendBroadcast(DKGEnvelope const &env);
  void SendCoefficients(std::vector<bn::Fr> const &a_i, std::vector<bn::Fr> const &b_i);
  void SendShares(std::vector<bn::Fr> const &a_i, std::vector<bn::Fr> const &b_i);
  void BroadcastComplaints();
  void BroadcastComplaintsAnswer();
  void BroadcastQualComplaints();
  void BroadcastReconstructionShares();
  /// @}

  /// @name Methods to check if enough messages have been received to trigger state transition
  /// @{
  void ReceivedCoefficientsAndShares();
  void ReceivedComplaint();
  void ReceivedComplaintsAnswer();
  void ReceivedQualShares();
  void ReceivedQualComplaint();
  void ReceivedReconstructionShares();
  /// @}

  /// @name Handlers for messages
  /// @{
  void OnNewCoefficients(std::shared_ptr<CoefficientsMessage> const &coefficients,
                         MuddleAddress const &                       from_id);
  void OnComplaints(std::shared_ptr<ComplaintsMessage> const &complaint,
                    MuddleAddress const &                     from_id);
  void OnExposedShares(std::shared_ptr<SharesMessage> const &shares, MuddleAddress const &from_id);
  void OnComplaintsAnswer(std::shared_ptr<SharesMessage> const &answer,
                          MuddleAddress const &                 from_id);
  void OnQualComplaints(std::shared_ptr<SharesMessage> const &shares, MuddleAddress const &from_id);
  void OnReconstructionShares(std::shared_ptr<SharesMessage> const &shares,
                              MuddleAddress const &                 from_id);
  /// @}

  /// @name Helper methods
  /// @{
  uint32_t                          CabinetIndex(MuddleAddress const &other_address) const;
  std::unordered_set<MuddleAddress> ComputeComplaints();
  void             CheckComplaintAnswer(std::shared_ptr<SharesMessage> const &answer,
                                        MuddleAddress const &from_id, uint32_t from_index);
  bool             BuildQual();
  SharesExposedMap ComputeQualComplaints();
  void             ComputeSecretShare();
  bool             RunReconstruction();
  void             ComputePublicKeys();
  /// @}

public:
  DistributedKeyGeneration(
      MuddleAddress address, CabinetMembers const &cabinet, uint32_t const &threshold,
      std::function<void(DKGEnvelop const &)> broadcast_callback,
      std::function<void(MuddleAddress const &, std::pair<std::string, std::string> const &)>
          rpc_callback);

  void   BroadcastShares();
  void   OnNewShares(MuddleAddress from_id, std::pair<MsgShare, MsgShare> const &shares);
  void   OnDkgMessage(MuddleAddress const &from, std::shared_ptr<DKGMessage> msg_ptr);
  void   ResetCabinet();
  void   SetDkgOutput(bn::G2 &public_key, bn::Fr &secret_share,
                      std::vector<bn::G2> &public_key_shares, std::set<MuddleAddress> &qual);
  bool   finished() const;
  bn::G2 group() const;
};
}  // namespace dkg
}  // namespace fetch
