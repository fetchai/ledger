#pragma once
#include "core/state_machine.hpp"

#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/muddle/subscription.hpp"

#include "beacon_protocol.hpp"
#include "beacon_round.hpp"
#include "beacon_setup_protocol.hpp"
#include "beacon_setup_service.hpp"
#include "cabinet_member_details.hpp"
#include "entropy.hpp"

#include <cstdint>
#include <deque>
#include <iostream>
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
    COMITEE_ROTATION
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
  using StateMachine      = core::StateMachine<State>;
  using StateMachinePtr   = std::shared_ptr<StateMachine>;
  using SignatureShare    = BeaconRoundDetails::SignatureShare;

  BeaconService(Endpoint &endpoint, CertificatePtr certificate);

  /// Maintainance logic
  /// @{
  /// @brief this function is called when the node is in the cabinet
  void StartNewCabinet(CabinetMemberList members, uint32_t threshold);

  /// @brief this function is called when the node is not in the next committee.
  void SkipRound();

  bool SwitchCabinet();
  /// @}

  State OnWaitForSetupCompletionState();
  State OnPrepareEntropyGeneration();

  State OnBroadcastSignatureState();
  State OnCollectSignaturesState();
  State OnCompleteState();

  State OnComiteeState();
  void  SubmitSignatureShare(uint64_t, uint64_t, SignatureShare);  // TODO: Add signature
  void  ScheduleEntropyGeneration();

  /// Beacon runnables
  /// @{
  std::weak_ptr<core::Runnable> GetMainRunnable();
  std::weak_ptr<core::Runnable> GetSetupRunnable();
  /// @}
private:
  std::mutex      mutex_;
  CertificatePtr  certificate_;
  Identity        identity_;
  Endpoint &      endpoint_;  ///< The muddle endpoint to communicate on
  StateMachinePtr state_machine_;

  /// Beacon and entropy control units
  /// @{
  std::deque<SharedBeacon> beacon_queue_;
  std::deque<Entropy>      entropy_queue_;
  std::deque<Entropy>      ready_entropy_queue_;
  uint64_t                 next_cabinet_generation_number_{0};
  uint64_t                 next_cabinet_number_{0};

  std::shared_ptr<BeaconRoundDetails> active_beacon_;
  Entropy                             current_entropy_;
  /// @}

  ServerPtr rpc_server_;

  BeaconSetupService         cabinet_creator_;
  BeaconSetupServiceProtocol cabinet_creator_protocol_;
  BeaconServiceProtocol      beacon_protocol_;
};

}  // namespace beacon
}  // namespace fetch