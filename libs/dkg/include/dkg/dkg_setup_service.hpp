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

#include "beacon/aeon.hpp"
#include "core/state_machine.hpp"
#include "dkg/dkg_complaints_manager.hpp"
#include "dkg/dkg_messages.hpp"
#include "network/muddle/rbc.hpp"
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
    IDLE,
    WAIT_FOR_DIRECT_CONNECTIONS,
    WAIT_FOR_READY_CONNECTIONS,
    WAIT_FOR_SHARE,
    WAIT_FOR_COMPLAINTS,
    WAIT_FOR_COMPLAINT_ANSWERS,
    WAIT_FOR_QUAL_SHARES,
    WAIT_FOR_QUAL_COMPLAINTS,
    WAIT_FOR_RECONSTRUCTION_SHARES,
    BEACON_READY
  };

  using ConstByteArray     = byte_array::ConstByteArray;
  using StateMachine       = core::StateMachine<State>;
  using StateMachinePtr    = std::shared_ptr<StateMachine>;
  using MuddleAddress      = ConstByteArray;
  using Identity           = crypto::Identity;
  using CabinetMembers     = std::set<Identity>;
  using Endpoint           = muddle::MuddleEndpoint;
  using RBC                = network::RBC;
  using MessageCoefficient = std::string;
  using MessageShare       = std::string;
  using SharesExposedMap = std::unordered_map<MuddleAddress, std::pair<MessageShare, MessageShare>>;
  using AeonExecutionUnit       = beacon::AeonExecutionUnit;
  using SharedAeonExecutionUnit = std::shared_ptr<AeonExecutionUnit>;
  using CallbackFunction        = std::function<void(SharedAeonExecutionUnit)>;

  DkgSetupService()                        = delete;
  DkgSetupService(DkgSetupService const &) = delete;
  DkgSetupService(DkgSetupService &&)      = delete;
  DkgSetupService(Endpoint &endpoint, Identity identity);

  /// State functions
  /// @{
  State OnIdle();
  State OnWaitForDirectConnections();
  State OnWaitForReadyConnections();
  State OnWaitForShares();
  State OnWaitForComplaints();
  State OnWaitForComplaintAnswers();
  State OnWaitForQualShares();
  State OnWaitForQualComplaints();
  State OnWaitForReconstructionShares();
  State OnBeaconReady();
  /// @}

  /// Setup management
  /// @{
  void QueueSetup(SharedAeonExecutionUnit beacon);
  void SetBeaconReadyCallback(CallbackFunction callback);
  /// @}

  std::weak_ptr<core::Runnable> GetWeakRunnable();

  void OnNewShares(MuddleAddress from_id, std::pair<MessageShare, MessageShare> const &shares);
  void OnDkgMessage(MuddleAddress const &from, std::shared_ptr<DKGMessage> msg_ptr);

protected:
  Identity                              identity_;
  Endpoint &                            endpoint_;
  std::shared_ptr<muddle::Subscription> shares_subscription;
  RBC                                   pre_dkg_rbc_;
  RBC                                   rbc_;

  std::mutex                          mutex_;
  CallbackFunction                    callback_function_;
  std::deque<SharedAeonExecutionUnit> aeon_exe_queue_;
  SharedAeonExecutionUnit             beacon_;

  std::shared_ptr<StateMachine> state_machine_;
  telemetry::GaugePtr<uint8_t>  dkg_state_gauge_;

  std::set<MuddleAddress>                                    connections_;
  std::unordered_map<MuddleAddress, std::set<MuddleAddress>> ready_connections_;

  // Managing complaints
  ComplaintsManager       complaints_manager_;
  ComplaintsAnswerManager complaints_answer_manager_;
  QualComplaintsManager   qual_complaints_manager_;

  // Counters for types of messages received
  std::set<MuddleAddress> shares_received_;
  std::set<MuddleAddress> coefficients_received_;
  std::set<MuddleAddress> qual_coefficients_received_;
  std::set<MuddleAddress> reconstruction_shares_received_;

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
