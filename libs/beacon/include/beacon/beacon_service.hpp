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

#include "core/state_machine.hpp"

#include "entropy/entropy_generator_interface.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"

#include "beacon/aeon.hpp"
#include "beacon/beacon_protocol.hpp"
#include "beacon/beacon_setup_service.hpp"
#include "beacon/event_manager.hpp"
#include "beacon/events.hpp"
#include "beacon/public_key_message.hpp"
#include "core/digest.hpp"
#include "storage/object_store.hpp"

#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/registry.hpp"

#include <cstdint>
#include <deque>
#include <iostream>
#include <queue>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace beacon {

class BeaconService : public ledger::EntropyGeneratorInterface
{
public:
  constexpr static char const *LOGGING_NAME = "BeaconService";

  enum class State
  {
    WAIT_FOR_SETUP_COMPLETION,
    PREPARE_ENTROPY_GENERATION,
    COLLECT_SIGNATURES,
    VERIFY_SIGNATURES,
    COMPLETE,
    CABINET_ROTATION,

    WAIT_FOR_PUBLIC_KEYS,
    OBSERVE_ENTROPY_GENERATION
  };

  using Identity                = crypto::Identity;
  using Prover                  = crypto::Prover;
  using ProverPtr               = std::shared_ptr<Prover>;
  using Certificate             = crypto::Prover;
  using CertificatePtr          = std::shared_ptr<Certificate>;
  using Address                 = muddle::Packet::Address;
  using BeaconManager           = dkg::BeaconManager;
  using SharedAeonExecutionUnit = std::shared_ptr<AeonExecutionUnit>;
  using Endpoint                = muddle::MuddleEndpoint;
  using MuddleInterface         = muddle::MuddleInterface;
  using Client                  = muddle::rpc::Client;
  using ClientPtr               = std::shared_ptr<Client>;
  using MuddleAddress           = byte_array::ConstByteArray;
  using ConstByteArray          = byte_array::ConstByteArray;
  using Server                  = fetch::muddle::rpc::Server;
  using ServerPtr               = std::shared_ptr<Server>;
  using SubscriptionPtr         = muddle::MuddleEndpoint::SubscriptionPtr;
  using StateMachine            = core::StateMachine<State>;
  using StateMachinePtr         = std::shared_ptr<StateMachine>;
  using SignatureShare          = AeonExecutionUnit::SignatureShare;
  using Serializer              = serializers::MsgPackSerializer;
  using SharedEventManager      = EventManager::SharedEventManager;
  using BlockEntropyPtr         = std::shared_ptr<beacon::BlockEntropy>;
  using DeadlineTimer           = fetch::moment::DeadlineTimer;
  using OldStateStore           = fetch::storage::ObjectStore<AeonExecutionUnit>;

  BeaconService()                      = delete;
  BeaconService(BeaconService const &) = delete;

  BeaconService(MuddleInterface &muddle, const CertificatePtr &certificate,
                BeaconSetupService &beacon_setup, SharedEventManager event_manager,
                bool load_and_reload_on_crash = false);

  /// @name Entropy Generator
  /// @{
  Status GenerateEntropy(uint64_t block_number, BlockEntropy &entropy) override;
  /// @}

  /// Beacon runnable
  /// @{
  std::weak_ptr<core::Runnable> GetWeakRunnable();
  void                          MostRecentSeen(uint64_t round);
  /// @}

  friend class BeaconServiceProtocol;

  struct SignatureInformation
  {
    uint64_t                                               round{uint64_t(-1)};
    std::map<BeaconManager::MuddleAddress, SignatureShare> threshold_signatures;
  };

protected:
  /// State methods
  /// @{
  State OnWaitForSetupCompletionState();
  State OnPrepareEntropyGeneration();

  State OnCollectSignaturesState();
  State OnVerifySignaturesState();
  State OnCompleteState();
  /// @}

  /// Protocol endpoints
  /// @{
  SignatureInformation GetSignatureShares(uint64_t round);
  /// @}

  mutable Mutex                       mutex_;
  CertificatePtr                      certificate_;
  bool                                load_and_reload_on_crash_{false};
  std::deque<SharedAeonExecutionUnit> aeon_exe_queue_;

private:
  bool AddSignature(SignatureShare share);

  Identity         identity_;
  MuddleInterface &muddle_;
  Endpoint &       endpoint_;
  StateMachinePtr  state_machine_;
  DeadlineTimer    timer_to_proceed_{"beacon:main"};

  // Limit run away entropy generation
  uint64_t entropy_lead_blocks_    = 2;
  uint64_t most_recent_round_seen_ = 0;

  /// General configuration
  /// @{
  bool broadcasting_ = false;
  /// @}

  /// Beacon and entropy control units
  /// @{
  std::shared_ptr<AeonExecutionUnit> active_exe_unit_;
  /// @}

  /// Variables relating to getting threshold signatures of the seed
  /// @{
  // Important this is ordered for trimming
  std::map<uint64_t, SignatureInformation> signatures_being_built_;
  std::size_t                              random_number_{0};
  Identity                                 qual_promise_identity_;
  service::Promise                         sig_share_promise_;

  BlockEntropyPtr block_entropy_previous_;
  BlockEntropyPtr block_entropy_being_created_;

  // Important this is ordered for trimming
  std::map<uint64_t, BlockEntropyPtr> completed_block_entropy_;
  /// @}

  ServerPtr           rpc_server_{nullptr};
  muddle::rpc::Client rpc_client_;

  /// Internal messaging
  /// @{
  SharedEventManager event_manager_;
  /// @}

  /// Distributed Key Generation
  /// @{
  BeaconServiceProtocol beacon_protocol_;
  /// @}

  /// Save keys so that recovery is possible in a crash situation
  /// @{
  OldStateStore old_state_;
  bool          OutOfSync();
  void          ReloadState();
  void          SaveState();
  /// @}

  //
  using Clock     = std::chrono::high_resolution_clock;
  using Timepoint = Clock::time_point;

  Timepoint started_request_for_sigs_;

  telemetry::CounterPtr         beacon_entropy_generated_total_;
  telemetry::CounterPtr         beacon_entropy_future_signature_seen_total_;
  telemetry::CounterPtr         beacon_entropy_forced_to_time_out_total_;
  telemetry::GaugePtr<uint64_t> beacon_entropy_last_requested_;
  telemetry::GaugePtr<uint64_t> beacon_entropy_last_generated_;
  telemetry::GaugePtr<uint64_t> beacon_entropy_current_round_;
  telemetry::GaugePtr<uint64_t> beacon_state_gauge_;
  telemetry::GaugePtr<uint64_t> beacon_most_recent_round_seen_;
  telemetry::HistogramPtr       beacon_collect_time_;
  telemetry::HistogramPtr       beacon_verify_time_;
};

}  // namespace beacon

namespace serializers {
template <typename D>
struct ArraySerializer<beacon::BeaconService::SignatureInformation, D>
{

public:
  using Type       = beacon::BeaconService::SignatureInformation;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &b)
  {
    auto array = array_constructor(2);
    array.Append(b.round);
    array.Append(b.threshold_signatures);
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &b)
  {
    array.GetNextValue(b.round);
    array.GetNextValue(b.threshold_signatures);
  }
};
}  // namespace serializers
}  // namespace fetch
