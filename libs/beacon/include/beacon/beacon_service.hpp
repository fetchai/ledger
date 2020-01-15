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

/**
 * The beacon service is responsible for generating entropy that the blocks can use. It is given the
 * output of DKG (if it was a successful miner) from the beacon setup service, and will continue to
 * generate entropy until that aeon is complete.
 *
 * It does not generate entropy more than N blocks ahead of the most recently seen block to prevent
 * certain attacks.
 *
 * Entropy gen normally happens in the following steps:
 *
 * 1. Get the 'aeon execution unit' for block N containing your threshold keys, the entropy of block
 * N - 1, and the aeon length M
 * 2. Prepare to generate entropy: populate SignatureInformation with your partial signature of N -
 * 1
 * 3. Request from a peer all the partial signatures they have for block N - 1
 * 4. Verify the response (this could contain all necessary sigs), if insufficient, return to 3.
 * 5. Prepare to generate entropy for block N + 1 if it's not greater than N + M
 *
 * There can be exceptions to this when recovering from a crash or synchronising to the chain.
 *
 */
class BeaconService : public ledger::EntropyGeneratorInterface
{
public:
  constexpr static char const *LOGGING_NAME = "BeaconService";

  enum class State
  {
    RELOAD_ON_STARTUP,
    WAIT_FOR_SETUP_COMPLETION,
    PREPARE_ENTROPY_GENERATION,
    COLLECT_SIGNATURES,
    VERIFY_SIGNATURES,
    COMPLETE,
    CABINET_ROTATION,
    WAIT_FOR_PUBLIC_KEYS,
    OBSERVE_ENTROPY_GENERATION
  };

  using SignatureShare = AeonExecutionUnit::SignatureShare;

  struct SignatureInformation
  {
    uint64_t                                                    round{uint64_t(-1)};
    std::map<dkg::BeaconManager::MuddleAddress, SignatureShare> threshold_signatures;
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
  using Serializer              = serializers::MsgPackSerializer;
  using SharedEventManager      = EventManager::SharedEventManager;
  using BlockEntropyPtr         = std::shared_ptr<beacon::BlockEntropy>;
  using DeadlineTimer           = fetch::moment::DeadlineTimer;
  using OldStateStore           = fetch::storage::ObjectStore<AeonExecutionUnit>;
  using StateStore              = fetch::storage::ObjectStore<byte_array::ConstByteArray>;
  using AllSigsStore            = fetch::storage::ObjectStore<SignatureInformation>;
  using SignaturesBeingBuilt    = std::map<uint64_t, SignatureInformation>;
  using CompletedBlockEntropy   = std::map<uint64_t, BlockEntropyPtr>;
  using ActiveExeUnit           = std::shared_ptr<AeonExecutionUnit>;
  using AeonExeQueue            = std::deque<SharedAeonExecutionUnit>;

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

  template <typename T, typename D>
  friend struct serializers::MapSerializer;

protected:
  /// State methods
  /// @{
  State OnReloadOnStartup();
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

  // Related to recovering state after a crash
  /// @{
  constexpr static uint16_t SAVE_PERIODICITY = 10;
  OldStateStore  old_state_;
  StateStore     saved_state_;
  AllSigsStore   saved_state_all_sigs_;
  void           ReloadState(State &next_state);
  void           SaveState();
  /// @}

  /// The state and functions that the beacon needs for operation is here. Thus,
  //  if it is recovered from file it needs these
  /// @{
  SignaturesBeingBuilt signatures_being_built_;

  // Important this is ordered for trimming - populated for external use
  // when creating blocks
  CompletedBlockEntropy completed_block_entropy_;

  ActiveExeUnit   active_exe_unit_;
  AeonExeQueue    aeon_exe_queue_;
  BlockEntropyPtr block_entropy_previous_;
  BlockEntropyPtr block_entropy_being_created_;
  /// @}

private:
  bool AddSignature(SignatureShare share);
  bool OutOfSync();

  mutable Mutex    mutex_;
  CertificatePtr   certificate_;
  Identity         identity_;
  MuddleInterface &muddle_;
  Endpoint &       endpoint_;
  StateMachinePtr  state_machine_;
  DeadlineTimer    timer_to_proceed_{"beacon:main"};
  bool             load_and_reload_on_crash_{false};

  // Limit run away entropy generation
  uint64_t entropy_lead_blocks_    = 2;
  uint64_t most_recent_round_seen_ = 0;

  /// Variables relating to getting threshold signatures of the seed
  /// @{
  std::size_t      random_number_{0};
  Identity         qual_promise_identity_;
  service::Promise sig_share_promise_;
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

  // Telemetry and debug
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

// Since it is annoying to serialize the state, an enum,
// we create a wrapper for serializing the beacon service
struct BeaconServiceSerializeWrapper
{
  BeaconService &beacon_service;
  uint16_t current_state{0};
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

// Note that this serializer saves the current state, and on deser will
// populate state_after_reload_
template <typename D>
struct MapSerializer<beacon::BeaconService, D>
{
public:
  using Type       = beacon::BeaconService;
  using DriverType = D;

  static uint8_t const ACTIVE_EXE_UNIT             = 1;
  static uint8_t const AEON_EXE_QUEUE              = 2;
  static uint8_t const BLOCK_ENTROPY_PREVIOUS      = 3;
  static uint8_t const BLOCK_ENTROPY_BEING_CREATED = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &beacon_service)
  {
    auto map = map_constructor(4);
    map.Append(ACTIVE_EXE_UNIT, beacon_service.active_exe_unit_);
    map.Append(AEON_EXE_QUEUE, beacon_service.aeon_exe_queue_);
    map.Append(BLOCK_ENTROPY_PREVIOUS, beacon_service.block_entropy_previous_);
    map.Append(BLOCK_ENTROPY_BEING_CREATED, beacon_service.block_entropy_being_created_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &beacon_service)
  {
    map.ExpectKeyGetValue(ACTIVE_EXE_UNIT, beacon_service.active_exe_unit_);
    map.ExpectKeyGetValue(AEON_EXE_QUEUE, beacon_service.aeon_exe_queue_);

    map.ExpectKeyGetValue(BLOCK_ENTROPY_PREVIOUS, beacon_service.block_entropy_previous_);
    map.ExpectKeyGetValue(BLOCK_ENTROPY_BEING_CREATED,
                          beacon_service.block_entropy_being_created_);
  }
};

template <typename D>
struct MapSerializer<beacon::BeaconServiceSerializeWrapper, D>
{
public:
  using Type       = beacon::BeaconServiceSerializeWrapper;
  using DriverType = D;

  static uint8_t const BEACON_SERVICE = 1;
  static uint8_t const CURRENT_STATE   = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &wrapper)
  {
    auto map = map_constructor(2);
    map.Append(BEACON_SERVICE, wrapper.beacon_service);
    map.Append(CURRENT_STATE, wrapper.current_state);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &wrapper)
  {
    map.ExpectKeyGetValue(BEACON_SERVICE, wrapper.beacon_service);
    map.ExpectKeyGetValue(CURRENT_STATE, wrapper.current_state);
  }
};

}  // namespace serializers
}  // namespace fetch
