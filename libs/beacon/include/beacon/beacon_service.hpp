#pragma once
#include "core/state_machine.hpp"

#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/muddle/subscription.hpp"

#include "beacon/beacon_protocol.hpp"
#include "beacon/beacon_round.hpp"
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

class BeaconService
{
public:
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

  using Identity          = crypto::Identity;
  using Prover            = crypto::Prover;
  using ProverPtr         = std::shared_ptr<Prover>;
  using Certificate       = crypto::Prover;
  using CertificatePtr    = std::shared_ptr<Certificate>;
  using Address           = muddle::Packet::Address;
  using BeaconManager     = dkg::BeaconManager;
  using SharedBeacon      = std::shared_ptr<BeaconRoundDetails>;
  using Endpoint          = muddle::MuddleEndpoint;
  using Muddle            = muddle::Muddle;
  using Client            = muddle::rpc::Client;
  using ClientPtr         = std::shared_ptr<Client>;
  using CabinetMemberList = std::unordered_set<Identity>;
  using ConstByteArray    = byte_array::ConstByteArray;
  using Server            = fetch::muddle::rpc::Server;
  using ServerPtr         = std::shared_ptr<Server>;
  using SubscriptionPtr   = muddle::MuddleEndpoint::SubscriptionPtr;
  using StateMachine      = core::StateMachine<State>;
  using StateMachinePtr   = std::shared_ptr<StateMachine>;
  using SignatureShare    = BeaconRoundDetails::SignatureShare;
  using Serializer        = serializers::MsgPackSerializer;

  BeaconService(Endpoint &endpoint, CertificatePtr certificate);

  /// Maintainance logic
  /// @{
  /// @brief this function is called when the node is in the cabinet
  void StartNewCabinet(CabinetMemberList members, uint32_t threshold, uint64_t round_start,
                       uint64_t round_end);
  /// @}

  State OnWaitForSetupCompletionState();
  State OnPrepareEntropyGeneration();
  State OnObserveEntropyGeneration();

  State OnBroadcastSignatureState();
  State OnCollectSignaturesState();
  State OnCompleteState();

  State OnComiteeState();
  void  SubmitSignatureShare(uint64_t round, SignatureShare);

  /// Beacon runnables
  /// @{
  std::weak_ptr<core::Runnable> GetMainRunnable();
  std::weak_ptr<core::Runnable> GetSetupRunnable();
  /// @}
private:
  std::mutex      mutex_;
  CertificatePtr  certificate_;
  Identity        identity_;
  Endpoint &      endpoint_;
  StateMachinePtr state_machine_;

  /// Beacon and entropy control units
  /// @{
  std::deque<SharedBeacon> beacon_queue_;
  Entropy                  next_entropy_{};
  std::deque<Entropy>      ready_entropy_queue_;

  std::shared_ptr<BeaconRoundDetails>             active_beacon_;
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