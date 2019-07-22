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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/containers/mapping.hpp"
#include "core/mutex.hpp"
#include "core/state_machine.hpp"
#include "dkg/dkg.hpp"
#include "dkg/dkg_rpc_protocol.hpp"
#include "dkg/rbc.hpp"
#include "dkg/round.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/consensus/entropy_generator_interface.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace fetch {
namespace muddle {

class MuddleEndpoint;
class Subscription;

}  // namespace muddle

namespace dkg {

/**
 * The DKG service is designed to provide the system with a reliable entropy source that can be
 * integrated into the staking mechanism.
 *
 * The DKG will build a set of keys for for a given block period called an aeon. During this aeon
 * signatures will be sent out from each participant on a round basis. These rounds roughly sync
 * up with block intervals. However, it should be noted that in general the DKG will run ahead of
 * the main chain.
 *
 * The following diagram outlines that basic state machine that is operating in the DKG service.
 *
 *                       ┌───────────────────────┐
 *                       │                       │
 *                       │       Start DKG       │◀ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
 *                       │                       │                         │
 *                       └───────────────────────┘
 *                                   │                                     │
 *                                   │                               At the start
 *                                   ▼                                of the next
 *                       ┌───────────────────────┐                       aeon
 *                       │                       │                         │
 *                       │Wait for DKG Completion│
 *                       │                       │                         │
 *                       └───────────────────────┘
 *                                   │                                     │
 *                                   │
 *                                   ▼                                     │
 *                       ┌───────────────────────┐
 *                       │                       │                         │
 *                       │  Broadcast Signature  │◀ ─ ─ ─ ─ ─ ┐
 *                       │                       │                         │
 *                       └───────────────────────┘            │
 *                                   │                                     │
 *                                   │                  At the start
 *                                   ▼                   of the next       │
 *                       ┌───────────────────────┐          round
 *                       │                       │                         │
 *                       │  Collect Signatures   │            │
 *                       │                       │                         │
 *                       └───────────────────────┘            │
 *                                   │                                     │
 *                                   │                        │
 *                                   ▼                                     │
 *                       ┌───────────────────────┐            │
 *                       │                       │                         │
 *                       │       Complete        │─ ─ ─ ─ ─ ─ ┴ ─ ─ ─ ─ ─ ─
 *                       │                       │
 *                       └───────────────────────┘
 */
class DkgService : public ledger::EntropyGeneratorInterface
{
public:
  static constexpr char const *LOGGING_NAME = "DkgService";

  enum class State
  {
    BUILD_AEON_KEYS,
    WAIT_FOR_DKG_COMPLETION,
    BROADCAST_SIGNATURE,
    COLLECT_SIGNATURES,
    COMPLETE,
  };

  using Endpoint       = muddle::MuddleEndpoint;
  using Digest         = ledger::Digest;
  using ConstByteArray = byte_array::ConstByteArray;
  using MuddleAddress  = ConstByteArray;
  using CabinetMembers = std::set<MuddleAddress>;
  using RBCMessageType = DKGEnvelop;

  // Construction / Destruction
  explicit DkgService(Endpoint &endpoint, ConstByteArray address);
  DkgService(DkgService const &) = delete;
  DkgService(DkgService &&)      = delete;
  ~DkgService() override         = default;

  /// @name External Events
  /// @{
  void SubmitSignatureShare(uint64_t round, uint32_t const &id, std::string const &signature);
  void SubmitShare(MuddleAddress const &address, std::pair<std::string, std::string> const &shares);
  void OnRbcDeliver(MuddleAddress const &from, byte_array::ConstByteArray const &payload);
  /// @}

  /// @name Entropy Generator
  /// @{
  Status GenerateEntropy(Digest block_digest, uint64_t block_number, uint64_t &entropy) override;
  /// @}

  /// @name Helper Methods
  /// @{
  std::weak_ptr<core::Runnable> GetWeakRunnable()
  {
    return state_machine_;
  }

  void ResetCabinet(CabinetMembers cabinet, uint32_t threshold = UINT32_MAX)
  {
    FETCH_LOCK(cabinet_lock_);
    current_cabinet_ = std::move(cabinet);
    if (threshold == UINT32_MAX)
    {
      current_threshold_ = static_cast<uint32_t>(current_cabinet_.size() / 2 - 1);
    }
    else
    {
      current_threshold_ = threshold;
    }
    assert(current_cabinet_.size() > 3 * threshold);  // To meet the requirements for the RBC
    id_ = static_cast<uint32_t>(
        std::distance(current_cabinet_.begin(), current_cabinet_.find(address_)));
    FETCH_LOG_INFO(LOGGING_NAME, "Resetting cabinet. Cabinet size: ", current_cabinet_.size(),
                   " threshold: ", threshold);
    dkg_.ResetCabinet();
    rbc_.ResetCabinet();
  }
  void SendShares(MuddleAddress const &                      destination,
                  std::pair<std::string, std::string> const &shares);
  void SendReliableBroadcast(RBCMessageType const &msg);
  /// @}

  // Operators
  DkgService &operator=(DkgService const &) = delete;
  DkgService &operator=(DkgService &&) = delete;

private:
  using Address         = ledger::Address;
  using SubscriptionPtr = std::shared_ptr<muddle::Subscription>;
  using AddressMapping  = core::Mapping<MuddleAddress, Address>;
  using EntropyHistory  = std::unordered_map<uint64_t, uint64_t>;
  using CabinetIds      = std::unordered_map<MuddleAddress, uint32_t>;
  using StateMachine    = core::StateMachine<State>;
  using StateMachinePtr = std::shared_ptr<StateMachine>;
  using RpcProtocolPtr  = std::unique_ptr<DkgRpcProtocol>;
  using Promise         = service::Promise;
  using RMutex          = std::recursive_mutex;
  using Mutex           = std::mutex;
  using SignaturePtr    = std::unique_ptr<bn::G1>;
  using SignatureMap    = std::map<uint64_t, SignaturePtr>;
  using RoundMap        = std::map<uint64_t, RoundPtr>;
  using PrivateKey      = bn::Fr;
  using PublicKey       = bn::G2;
  using PublicKeyList   = std::vector<bn::G2>;

  struct Submission
  {
    uint64_t round;
    uint32_t id;
    bn::G1   signature;
  };

  using SubmissionList = std::deque<Submission>;

  /// @name State Handlers
  /// @{
  State OnBuildAeonKeysState();
  State OnWaitForDkgCompletionState();
  State OnBroadcastSignatureState();
  State OnCollectSignaturesState();
  State OnCompleteState();
  /// @}

  /// @name Utils
  /// @{
  bool     GetSignaturePayload(uint64_t round, ConstByteArray &payload);
  RoundPtr LookupRound(uint64_t round, bool create = false);
  /// @}

  ConstByteArray const     address_;        ///< Our muddle address
  uint32_t                 id_;             ///< Our DKG ID (derived from index in current_cabinet_)
  Endpoint &               endpoint_;       ///< The muddle endpoint to communicate on
  muddle::rpc::Server      rpc_server_;     ///< The services' RPC server
  muddle::rpc::Client      rpc_client_;     ///< The services' RPC client
  RpcProtocolPtr           rpc_proto_;      ///< The services RPC protocol
  StateMachinePtr          state_machine_;  ///< The service state machine
  rbc::RBC                 rbc_;            ///< Runs the RBC protocol
  DistributedKeyGeneration dkg_;            ///< Runs DKG protocol

  /// @name State Machine Data
  /// @{
  static bn::G2 group_g_;
  PrivateKey    aeon_secret_share_;  ///< The current secret share for the aeon
  PublicKey     aeon_public_key_;    ///< The public key for our secret share
  CabinetMembers
                aeon_qual_set_;  ///< The set of muddle addresses which successfully completed the DKG
  PublicKeyList aeon_public_key_shares_;  ///< The public keys for DKG qualified set
  /// @}

  /// @name Cabinet / Aeon Data
  /// @{
  mutable RMutex cabinet_lock_{};        // Priority 1.
  uint32_t       current_threshold_{1};  ///< The current threshold for the aeon
  CabinetMembers current_cabinet_{};     ///< The set of muddle addresses of the cabinet
  /// @}

  /// @name Round Data
  /// @{
  mutable RMutex        round_lock_{};                 // Priority 3.
  SubmissionList        pending_signatures_{};         ///< The queue of pending signatures
  std::atomic<uint64_t> earliest_completed_round_{0};  ///< The round idx for the next entropy req
  std::atomic<uint64_t> current_round_{0};             ///< The current round being generated
  RoundMap              rounds_{};                     ///< The map of round data
                                                       /// @}
};

}  // namespace dkg
}  // namespace fetch
