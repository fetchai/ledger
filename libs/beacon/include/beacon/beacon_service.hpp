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
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/muddle/subscription.hpp"

#include "beacon/aeon.hpp"
#include "beacon/beacon_protocol.hpp"
#include "beacon/beacon_setup_protocol.hpp"
#include "beacon/beacon_setup_service.hpp"
#include "beacon/cabinet_member_details.hpp"
#include "beacon/entropy.hpp"

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
  using Muddle                  = muddle::Muddle;
  using Client                  = muddle::rpc::Client;
  using ClientPtr               = std::shared_ptr<Client>;
  using CabinetMemberList       = std::unordered_set<Identity>;
  using ConstByteArray          = byte_array::ConstByteArray;
  using Server                  = fetch::muddle::rpc::Server;
  using ServerPtr               = std::shared_ptr<Server>;
  using SubscriptionPtr         = muddle::MuddleEndpoint::SubscriptionPtr;
  using StateMachine            = core::StateMachine<State>;
  using StateMachinePtr         = std::shared_ptr<StateMachine>;
  using SignatureShare          = AeonExecutionUnit::SignatureShare;
  using Serializer              = serializers::MsgPackSerializer;
  using Digest                  = ledger::Digest;

  BeaconService()                      = delete;
  BeaconService(BeaconService const &) = delete;

  BeaconService(Endpoint &endpoint, CertificatePtr certificate);

  /// @name Entropy Generator
  /// @{
  Status GenerateEntropy(Digest block_digest, uint64_t block_number, uint64_t &entropy) override;
  /// @}

  /// Maintainance logic
  /// @{
  /// @brief this function is called when the node is in the cabinet
  void StartNewCabinet(CabinetMemberList members, uint32_t threshold, uint64_t round_start,
                       uint64_t round_end);
  /// @}

  /// Beacon runnables
  /// @{
  std::weak_ptr<core::Runnable> GetMainRunnable();
  std::weak_ptr<core::Runnable> GetSetupRunnable();
  /// @}

  template <typename T>
  friend class core::StateMachine;
  friend class BeaconServiceProtocol;

protected:
  /// State methods
  /// @{
  State OnWaitForSetupCompletionState();
  State OnPrepareEntropyGeneration();
  State OnObserveEntropyGeneration();

  State OnBroadcastSignatureState();
  State OnCollectSignaturesState();
  State OnCompleteState();

  State OnComiteeState();
  /// @}

  /// Protocol endpoints
  /// @{
  void SubmitSignatureShare(uint64_t round, SignatureShare);
  /// @}
private:
  bool AddSignature(SignatureShare share)
  {
    assert(active_exe_unit_ != nullptr);

    auto ret = active_exe_unit_->manager.AddSignaturePart(share.identity, share.public_key,
                                                          share.signature);
    if (ret == BeaconManager::AddResult::INVALID_SIGNATURE)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Signature invalid.");

      // TODO: Received invalid signature - deal with it.

      return false;
    }
    else if (ret == BeaconManager::AddResult::NOT_MEMBER)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Signature from non-member.");

      // TODO: Received signature from non-member - deal with it.
      return false;
    }
    return true;
  }

  std::mutex      mutex_;
  CertificatePtr  certificate_;
  Identity        identity_;
  Endpoint &      endpoint_;
  StateMachinePtr state_machine_;
  uint64_t        blocks_per_round_{5};  // TODO: Make configurable

  /// Beacon and entropy control units
  /// @{
  std::deque<SharedAeonExecutionUnit> aeon_exe_queue_;
  Entropy                             next_entropy_{};
  std::deque<Entropy>                 ready_entropy_queue_;

  std::shared_ptr<AeonExecutionUnit>              active_exe_unit_;
  Entropy                                         current_entropy_;
  std::deque<std::pair<uint64_t, SignatureShare>> signature_queue_;
  /// @}

  /// Observing beacon
  /// @{
  SubscriptionPtr              entropy_subscription_;
  std::priority_queue<Entropy> incoming_entropy_;
  /// @}

  ServerPtr           rpc_server_;
  muddle::rpc::Client rpc_client_;

  /// Distributed Key Generation
  /// @{
  BeaconSetupService         cabinet_creator_;
  BeaconSetupServiceProtocol cabinet_creator_protocol_;
  BeaconServiceProtocol      beacon_protocol_;
  /// @}
};

}  // namespace beacon
}  // namespace fetch
