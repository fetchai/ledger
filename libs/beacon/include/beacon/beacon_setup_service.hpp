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

#include "beacon/beacon_complaints_manager.hpp"
#include "beacon/notarisation_manager.hpp"
#include "core/service_ids.hpp"
#include "core/state_machine.hpp"
#include "dkg/dkg_messages.hpp"
#include "moment/clocks.hpp"
#include "moment/deadline_timer.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/subscription.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/telemetry.hpp"

#include "muddle/punishment_broadcast_channel.hpp"
#include "muddle/rbc.hpp"

#include "beacon/aeon.hpp"

namespace fetch {
namespace muddle {

class MuddleInterface;

}  // namespace muddle

namespace shards {

class ManifestCacheInterface;

}  // namespace shards

namespace beacon {

struct DryRunInfo
{
  crypto::mcl::PublicKey            public_key;
  AeonExecutionUnit::SignatureShare sig_share;
};

class BeaconSetupService
{
public:
  static constexpr char const *LOGGING_NAME = "BeaconSetupService";

  // Note: these must be maintained in the order which they are called
  enum class State : uint8_t
  {
    IDLE,
    RESET,
    CONNECT_TO_ALL,
    WAIT_FOR_READY_CONNECTIONS,
    WAIT_FOR_NOTARISATION_KEYS,
    WAIT_FOR_SHARES,
    WAIT_FOR_COMPLAINTS,
    WAIT_FOR_COMPLAINT_ANSWERS,
    WAIT_FOR_QUAL_SHARES,
    WAIT_FOR_QUAL_COMPLAINTS,
    WAIT_FOR_RECONSTRUCTION_SHARES,
    COMPUTE_PUBLIC_SIGNATURE,
    DRY_RUN_SIGNING,
    BEACON_READY
  };

  using ConstByteArray  = byte_array::ConstByteArray;
  using StateMachine    = core::StateMachine<State>;
  using StateMachinePtr = std::shared_ptr<StateMachine>;
  using MuddleAddress   = ConstByteArray;
  using Identity        = crypto::Identity;

  using CabinetMembers          = std::set<Identity>;
  using CabinetMemberList       = std::set<MuddleAddress>;
  using MuddleEndpoint          = muddle::MuddleEndpoint;
  using MuddleInterface         = muddle::MuddleInterface;
  using ManifestCacheInterface  = shards::ManifestCacheInterface;
  using RBC                     = muddle::RBC;
  using ReliableChannel         = muddle::BroadcastChannelInterface;
  using ReliableChannelPtr      = std::unique_ptr<ReliableChannel>;
  using SubscriptionPtr         = muddle::MuddleEndpoint::SubscriptionPtr;
  using SharedAeonExecutionUnit = std::shared_ptr<AeonExecutionUnit>;
  using CallbackFunction        = std::function<void(SharedAeonExecutionUnit)>;
  using ComplaintsManager       = beacon::ComplaintsManager;
  using ComplaintAnswersManager = beacon::ComplaintAnswersManager;
  using QualComplaintsManager   = beacon::QualComplaintsManager;
  using BeaconManager           = dkg::BeaconManager;
  using DKGMessage              = dkg::DKGMessage;
  using DKGEnvelope             = dkg::DKGEnvelope;
  using ComplaintsMessage       = dkg::ComplaintsMessage;
  using FinalStateMessage       = dkg::FinalStateMessage;
  using NotarisationKeyMessage  = dkg::NotarisationKeyMessage;
  using CoefficientsMessage     = dkg::CoefficientsMessage;
  using ConnectionsMessage      = dkg::ConnectionsMessage;
  using SharesMessage           = dkg::SharesMessage;
  using DKGSerializer           = dkg::DKGSerializer;
  using MessageCoefficient      = DKGMessage::Coefficient;
  using MessageShare            = DKGMessage::Share;
  using SharesExposedMap = std::unordered_map<MuddleAddress, std::pair<MessageShare, MessageShare>>;
  using DeadlineTimer    = moment::DeadlineTimer;
  using Clock            = moment::ClockPtr;
  using SignatureShare   = AeonExecutionUnit::SignatureShare;
  using GroupPubKeyPlusSigShare      = std::pair<std::string, SignatureShare>;
  using CertificatePtr               = std::shared_ptr<dkg::BeaconManager::Certificate>;
  using NotarisationManager          = ledger::NotarisationManager;
  using SharedNotarisationManager    = std::shared_ptr<NotarisationManager>;
  using NotarisationCallbackFunction = std::function<void(SharedNotarisationManager)>;

  BeaconSetupService(MuddleInterface &muddle, ManifestCacheInterface &manifest_cache,
                     CertificatePtr certificate);
  BeaconSetupService(BeaconSetupService const &) = delete;
  BeaconSetupService(BeaconSetupService &&)      = delete;
  virtual ~BeaconSetupService()                  = default;

  /// State functions
  /// @{
  State OnIdle();
  State OnReset();
  State OnConnectToAll();
  State OnWaitForReadyConnections();
  State OnWaitForNotarisationKeys();
  State OnWaitForShares();
  State OnWaitForComplaints();
  State OnWaitForComplaintAnswers();
  State OnWaitForQualShares();
  State OnWaitForQualComplaints();
  State OnWaitForReconstructionShares();
  State OnComputePublicSignature();
  State OnDryRun();
  State OnBeaconReady();
  /// @}

  /// Setup management
  /// @{
  void StartNewCabinet(CabinetMemberList members, uint32_t threshold, uint64_t round_start,
                       uint64_t round_end, uint64_t start_time, BlockEntropy const &prev_entropy);
  void Abort(uint64_t abort_below);
  void SetBeaconReadyCallback(CallbackFunction callback);
  void SetNotarisationCallback(NotarisationCallbackFunction callback);
  /// @}

  std::vector<std::weak_ptr<core::Runnable>> GetWeakRunnables();

protected:
  Identity                identity_;
  ManifestCacheInterface &manifest_cache_;
  MuddleInterface &       muddle_;
  MuddleEndpoint &        endpoint_;
  SubscriptionPtr         shares_subscription_;

  CertificatePtr     certificate_;
  ReliableChannelPtr rbc_;

  std::shared_ptr<StateMachine> state_machine_;
  std::set<MuddleAddress>       connections_;

  // Managing complaints
  ComplaintsManager       complaints_manager_;
  ComplaintAnswersManager complaint_answers_manager_;
  QualComplaintsManager   qual_complaints_manager_;

  // Counters for types of messages received
  std::set<MuddleAddress>                   shares_received_;
  std::set<MuddleAddress>                   coefficients_received_;
  std::set<MuddleAddress>                   qual_coefficients_received_;
  std::map<MuddleAddress, SharesExposedMap> reconstruction_shares_received_;
  std::set<MuddleAddress>                   valid_dkg_members_;

  /// @name Methods to send messages
  /// @{
  void         SendBroadcast(DKGEnvelope const &env);
  void         BroadcastNotarisationKeys();
  virtual void BroadcastShares();
  virtual void BroadcastComplaints();
  virtual void BroadcastComplaintAnswers();
  virtual void BroadcastQualCoefficients();
  virtual void BroadcastQualComplaints();
  virtual void BroadcastReconstructionShares();
  /// @}

  /// @name Helper function
  /// @{
  std::set<MuddleAddress> ComputeComplaints();
  /// @}

  // Telemetry
  telemetry::GaugePtr<uint64_t> beacon_dkg_state_gauge_;
  telemetry::GaugePtr<uint64_t> beacon_dkg_connections_gauge_;
  telemetry::GaugePtr<uint64_t> beacon_dkg_all_connections_gauge_;
  telemetry::GaugePtr<uint64_t> beacon_dkg_failures_required_to_complete_;
  telemetry::GaugePtr<uint64_t> beacon_dkg_state_failed_on_;
  telemetry::GaugePtr<uint64_t> beacon_dkg_time_allocated_;
  telemetry::GaugePtr<uint64_t> beacon_dkg_reference_timepoint_;
  telemetry::GaugePtr<uint64_t> beacon_dkg_aeon_setting_up_;
  telemetry::GaugePtr<uint64_t> beacon_dkg_miners_in_qual_;
  telemetry::CounterPtr         beacon_dkg_failures_total_;
  telemetry::CounterPtr         beacon_dkg_aborts_total_;
  telemetry::CounterPtr         beacon_dkg_successes_total_;

  // Members below protected by mutex
  std::mutex                   mutex_;
  CallbackFunction             callback_function_;
  NotarisationCallbackFunction notarisation_callback_function_;

  std::deque<SharedAeonExecutionUnit>                        aeon_exe_queue_;
  SharedAeonExecutionUnit                                    beacon_;
  SharedNotarisationManager                                  notarisation_manager_;
  std::unordered_map<MuddleAddress, std::set<MuddleAddress>> ready_connections_;
  std::map<MuddleAddress, const NotarisationKeyMessage>      notarisation_key_msgs_;

  std::map<MuddleAddress, ConstByteArray> final_state_payload_;

private:
  uint64_t abort_below_ = 0;

  // Timing management
  void          SetTimeToProceed(State state);
  Clock         system_clock_ = moment::GetClock("beacon:dkg_system", moment::ClockType::SYSTEM);
  Clock         clock_        = moment::GetClock("beacon:dkg", moment::ClockType::SYSTEM);
  DeadlineTimer timer_to_proceed_{"beacon:dkg"};

  static constexpr double MAX_DKG_BOUND_MULTIPLE = 4.0;
  uint64_t reference_timepoint_ = 0;  ///< A starting point in time all members use to syncronise
  uint64_t state_deadline_      = 0;  ///< The time at which the current state should move on
  uint64_t expected_dkg_timespan_ =
      0;  ///< The expected time it will take for all DKG states to complete
  bool condition_to_proceed_ = false;  ///< Whether the state the DKG is in has been successful

  const std::map<BeaconSetupService::State, double> time_slot_map_;
  double                                            time_slots_in_dkg_ = 0;

  uint16_t failures_{0};

  // Debug/logging
  std::atomic<BeaconManager::CabinetIndex> index_{
      std::numeric_limits<BeaconManager::CabinetIndex>::max()};
  std::string NodeString();

  // Convenience functions
  ReliableChannelPtr ReliableBroadcastFactory();

  /// @name Handlers for messages
  /// @{
  void OnDkgMessage(MuddleAddress const &from, const std::shared_ptr<DKGMessage> &msg_ptr);
  void OnNewShares(const MuddleAddress &from, std::pair<MessageShare, MessageShare> const &shares);
  void OnNewSharesPacket(muddle::Packet const &packet, MuddleAddress const &last_hop);
  void OnNewCoefficients(CoefficientsMessage const &msg, MuddleAddress const &from);
  void OnComplaints(ComplaintsMessage const &msg, MuddleAddress const &from);
  void OnExposedShares(SharesMessage const &shares, MuddleAddress const &from_id);
  void OnComplaintAnswers(SharesMessage const &answer, MuddleAddress const &from);
  void OnQualComplaints(SharesMessage const &shares_msg, MuddleAddress const &from);
  void OnReconstructionShares(SharesMessage const &shares_msg, MuddleAddress const &from);
  void OnNotarisationKey(NotarisationKeyMessage const &key_msg, MuddleAddress const &from);
  /// @}

  /// @name Helper methods
  /// @{
  void     SetDeadlineForState(BeaconSetupService::State const &state);
  bool     BasicMsgCheck(MuddleAddress const &from, std::shared_ptr<DKGMessage> const &msg_ptr);
  void     CheckComplaintAnswers();
  bool     BuildQual();
  void     CheckQualComplaints();
  uint64_t PreDKGThreshold();
  uint32_t QualSize();
  /// @}
};
}  // namespace beacon

namespace serializers {
template <typename D>
struct ArraySerializer<beacon::DryRunInfo, D>
{
public:
  using Type       = beacon::DryRunInfo;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &b)
  {
    auto array = array_constructor(2);
    array.Append(b.public_key);
    array.Append(b.sig_share);
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &b)
  {
    array.GetNextValue(b.public_key);
    array.GetNextValue(b.sig_share);
  }
};
}  // namespace serializers
}  // namespace fetch
