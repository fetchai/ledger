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

#include "core/state_machine.hpp"
#include "dkg/dkg_complaints_manager.hpp"
#include "dkg/dkg_manager.hpp"
#include "dkg/dkg_messages.hpp"
#include "network/muddle/rpc/client.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/telemetry.hpp"

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
class DkgSetupService
{
public:
  static constexpr char const *LOGGING_NAME = "DkgSetupService";

  enum class State : uint8_t
  {
    INITIAL,
    WAITING_FOR_SHARE,
    WAITING_FOR_COMPLAINTS,
    WAITING_FOR_COMPLAINT_ANSWERS,
    WAITING_FOR_QUAL_SHARES,
    WAITING_FOR_QUAL_COMPLAINTS,
    WAITING_FOR_RECONSTRUCTION_SHARES,
    FINAL
  };

  using StateMachine       = core::StateMachine<State>;
  using StateMachinePtr    = std::shared_ptr<StateMachine>;
  using MuddleAddress      = byte_array::ConstByteArray;
  using CabinetMembers     = std::set<MuddleAddress>;
  using Endpoint           = muddle::MuddleEndpoint;
  using MessageCoefficient = std::string;
  using MessageShare       = std::string;
  using SharesExposedMap = std::unordered_map<MuddleAddress, std::pair<MessageShare, MessageShare>>;

  DkgSetupService(
      MuddleAddress address, std::function<void(DKGEnvelope const &)> broadcast_function,
      std::function<void(MuddleAddress const &, std::pair<std::string, std::string> const &)>
          rpc_function);

  /// State functions
  /// @{
  State OnInitial();
  State OnWaitForShares();
  State OnWaitForComplaints();
  State OnWaitForComplaintAnswers();
  State OnWaitForQualShares();
  State OnWaitForQualComplaints();
  State OnWaitForReconstructionShares();
  State OnFinal();
  /// @}

  std::weak_ptr<core::Runnable> GetWeakRunnable();

  void OnNewShares(MuddleAddress from_id, std::pair<MessageShare, MessageShare> const &shares);
  void OnDkgMessage(MuddleAddress const &from, std::shared_ptr<DKGMessage> msg_ptr);

  void ResetCabinet(CabinetMembers const &cabinet, uint32_t threshold);
  bool finished() const;
  void SetDkgOutput(bn::G2 &public_key, bn::Fr &secret_share,
                    std::vector<bn::G2> &public_key_shares, std::set<MuddleAddress> &qual);

protected:
  CabinetMembers                           cabinet_;  ///< Muddle addresses of cabinet members
  MuddleAddress                            address_;  ///< Our muddle address
  std::function<void(DKGEnvelope const &)> broadcast_function_;
  std::function<void(MuddleAddress const &, std::pair<std::string, std::string> const &)>
                               rpc_function_;
  telemetry::GaugePtr<uint8_t> dkg_state_gauge_;
  std::atomic<State>           state_{State::INITIAL};
  std::mutex                   mutex_;

  // Managing complaints
  ComplaintsManager       complaints_manager_;
  ComplaintsAnswerManager complaints_answer_manager_;
  QualComplaintsManager   qual_complaints_manager_;

  // Counters for types of messages received
  std::atomic<uint32_t>   shares_received_{0};
  std::atomic<uint32_t>   C_ik_received_{0};
  std::set<MuddleAddress> A_ik_received_;
  std::set<MuddleAddress> reconstruction_shares_received_;

  std::shared_ptr<StateMachine> state_machine_;
  std::set<MuddleAddress>       qual_;  ///< Set of cabinet members who completed DKG
  DkgManager                    manager_;

  /// @name Methods to send messages
  /// @{
  void         SendBroadcast(DKGEnvelope const &env);
  virtual void BroadcastShares();
  virtual void BroadcastComplaints();
  virtual void BroadcastComplaintsAnswer();
  virtual void BroadcastQualCoefficients();
  virtual void BroadcastQualComplaints();
  virtual void BroadcastReconstructionShares();
  /// @}

  /// @name Handlers for messages
  /// @{
  void OnNewCoefficients(CoefficientsMessage const &coefficients, MuddleAddress const &from_id);
  void OnComplaints(ComplaintsMessage const &complaint, MuddleAddress const &from_id);
  void OnExposedShares(SharesMessage const &shares, MuddleAddress const &from_id);
  void OnComplaintsAnswer(SharesMessage const &answer, MuddleAddress const &from_id);
  void OnQualComplaints(SharesMessage const &shares, MuddleAddress const &from_id);
  void OnReconstructionShares(SharesMessage const &shares, MuddleAddress const &from_id);
  /// @}

  /// @name Helper methods
  /// @{
  bool BasicMsgCheck(MuddleAddress const &from, std::shared_ptr<DKGMessage> const &msg_ptr);
  void CheckComplaintAnswer(SharesMessage const &answer, MuddleAddress const &from_id);
  bool BuildQual();
  void CheckQualComplaints();
  /// @}
};
}  // namespace dkg
}  // namespace fetch
