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
#include "beacon/event_manager.hpp"
#include "beacon/events.hpp"
#include "beacon/verification_vector_message.hpp"

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

    WAIT_FOR_VERIFICATION_VECTORS,
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
  using SharedEventManager      = EventManager::SharedEventManager;

  BeaconService()                      = delete;
  BeaconService(BeaconService const &) = delete;

  BeaconService(Endpoint &endpoint, CertificatePtr certificate, SharedEventManager event_manager,
                uint64_t blocks_per_round = 1);

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

  State OnBroadcastSignatureState();
  State OnCollectSignaturesState();
  State OnCompleteState();

  State OnComiteeState();

  State OnWaitForVerificationVectors();
  State OnObserveEntropyGeneration();
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

    // Checking that the signature is valid
    if (ret == BeaconManager::AddResult::INVALID_SIGNATURE)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Signature invalid.");

      EventInvalidSignature event;
      // TODO(tfr): Received invalid signature - fill event details
      event_manager_->Dispatch(event);

      return false;
    }
    else if (ret == BeaconManager::AddResult::NOT_MEMBER)
    {  // And that it was sent by a member of the cabinet
      FETCH_LOG_ERROR(LOGGING_NAME, "Signature from non-member.");

      EventSignatureFromNonMember event;
      // TODO(tfr): Received signature from non-member - deal with it.
      event_manager_->Dispatch(event);

      return false;
    }
    return true;
  }

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
  SubscriptionPtr                                verification_vec_subscription_{nullptr};
  std::priority_queue<VerificationVectorMessage> incoming_verification_vectors_{};
  SubscriptionPtr                                entropy_subscription_{nullptr};
  std::priority_queue<Entropy>                   incoming_entropy_{};
  /// @}

  ServerPtr           rpc_server_{nullptr};
  muddle::rpc::Client rpc_client_;

  /// Internal messaging
  /// @{
  SharedEventManager event_manager_;
  /// @}

  /// Distributed Key Generation
  /// @{
  BeaconSetupService         cabinet_creator_;
  BeaconSetupServiceProtocol cabinet_creator_protocol_;
  BeaconServiceProtocol      beacon_protocol_;
  /// @}
};

}  // namespace beacon
}  // namespace fetch
