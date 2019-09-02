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

#include "ledger/consensus/entropy_generator_interface.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"

#include "beacon/aeon.hpp"
#include "beacon/beacon_protocol.hpp"
#include "beacon/beacon_setup_service.hpp"
#include "beacon/cabinet_member_details.hpp"
#include "beacon/entropy.hpp"
#include "beacon/event_manager.hpp"
#include "beacon/events.hpp"
#include "beacon/public_key_message.hpp"

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
    BROADCAST_SIGNATURE,
    COLLECT_SIGNATURES,
    COMPLETE,
    COMITEE_ROTATION,

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
  using CabinetMemberList       = std::set<Identity>;
  using ConstByteArray          = byte_array::ConstByteArray;
  using Server                  = fetch::muddle::rpc::Server;
  using ServerPtr               = std::shared_ptr<Server>;
  using SubscriptionPtr         = muddle::MuddleEndpoint::SubscriptionPtr;
  using StateMachine            = core::StateMachine<State>;
  using StateMachinePtr         = std::shared_ptr<StateMachine>;
  using SignatureShare          = AeonExecutionUnit::SignatureShare;
  using Serializer              = serializers::MsgPackSerializer;
  using Digest                  = ledger::Digest;
  using SharedEventManager      = EventManager::SharedEventManager;
  using BeaconSetupService      = beacon::BeaconSetupService;

  BeaconService()                      = delete;
  BeaconService(BeaconService const &) = delete;

  BeaconService(MuddleInterface &muddle, ledger::ManifestCacheInterface &manifest_cache,
                CertificatePtr certificate, SharedEventManager event_manager,
                uint64_t blocks_per_round = 1);

  /// @name Entropy Generator
  /// @{
  Status GenerateEntropy(Digest block_digest, uint64_t block_number, uint64_t &entropy) override;
  /// @}

  /// Maintainance logic
  /// @{
  /// @brief this function is called when the node is in the cabinet
  void StartNewCabinet(CabinetMemberList members, uint32_t threshold, uint64_t round_start,
                       uint64_t round_end, uint64_t last_block_time);
  /// @}

  /// Beacon runnables
  /// @{
  std::weak_ptr<core::Runnable> GetMainRunnable();
  std::weak_ptr<core::Runnable> GetSetupRunnable();
  /// @}

  friend class BeaconServiceProtocol;

protected:
  /// State methods
  /// @{
  State OnWaitForSetupCompletionState();
  State OnPrepareEntropyGeneration();

  State OnBroadcastSignatureState();
  State OnCollectSignaturesState();
  State OnCompleteState();

  State OnComiteeState();

  State OnWaitForPublicKeys();
  State OnObserveEntropyGeneration();
  /// @}

  /// Protocol endpoints
  /// @{
  void SubmitSignatureShare(uint64_t round, SignatureShare);
  /// @}

private:
  bool AddSignature(SignatureShare share);

  std::mutex      mutex_;
  CertificatePtr  certificate_;
  Identity        identity_;
  Endpoint &      endpoint_;
  StateMachinePtr state_machine_;

  /// General configuration
  /// @{
  uint64_t blocks_per_round_;
  /// @}

  /// Beacon and entropy control units
  /// @{
  std::deque<SharedAeonExecutionUnit> aeon_exe_queue_;

  std::deque<Entropy> ready_entropy_queue_;
  Entropy             latest_entropy_;

  std::shared_ptr<AeonExecutionUnit>              active_exe_unit_;
  Entropy                                         next_entropy_{};
  Entropy                                         current_entropy_;
  std::deque<std::pair<uint64_t, SignatureShare>> signature_queue_;
  /// @}

  /// Observing beacon
  /// @{
  SubscriptionPtr                       group_public_key_subscription_{nullptr};
  std::priority_queue<PublicKeyMessage> incoming_group_public_keys_{};
  SubscriptionPtr                       entropy_subscription_{nullptr};
  std::priority_queue<Entropy>          incoming_entropy_{};
  /// @}

  ServerPtr           rpc_server_{nullptr};
  muddle::rpc::Client rpc_client_;

  /// Internal messaging
  /// @{
  SharedEventManager event_manager_;
  /// @}

  /// Distributed Key Generation
  /// @{
  BeaconSetupService    cabinet_creator_;
  BeaconServiceProtocol beacon_protocol_;
  /// @}

  telemetry::CounterPtr entropy_generated_count_;
};

}  // namespace beacon
}  // namespace fetch
