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
#include "crypto/bls_base.hpp"
#include "dkg/dkg_rpc_protocol.hpp"
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
 *                       │      Build Keys       │◀ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
 *                       │                       │                         │
 *                       └───────────────────────┘
 *                                   │                                     │
 *                                   │                               At the start
 *                                   ▼                                of the next
 *                       ┌───────────────────────┐                       aeon
 *                       │                       │                         │
 *                       │  Request Secret Key   │
 *                       │                       │                         │
 *                       └───────────────────────┘
 *                                   │                                     │
 *                                   │
 *                                   ▼                                     │
 *                       ┌───────────────────────┐
 *                       │                       │                         │
 *                       │  Wait for Secret Key  │
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
  enum class State
  {
    BUILD_AEON_KEYS,
    REQUEST_SECRET_KEY,
    WAIT_FOR_SECRET_KEY,
    BROADCAST_SIGNATURE,
    COLLECT_SIGNATURES,
    COMPLETE,
  };

  using Endpoint       = muddle::MuddleEndpoint;
  using Digest         = ledger::Digest;
  using ConstByteArray = byte_array::ConstByteArray;
  using MuddleAddress  = ConstByteArray;
  using CabinetMembers = std::unordered_set<MuddleAddress>;

  // Construction / Destruction
  explicit DkgService(Endpoint &endpoint, ConstByteArray address, ConstByteArray dealer_address);
  DkgService(DkgService const &) = delete;
  DkgService(DkgService &&)      = delete;
  ~DkgService() override         = default;

  /// @name External Events
  /// @{
  struct SecretKeyReq
  {
    bool                    success{false};
    crypto::bls::PrivateKey secret_share{};
    crypto::bls::PublicKey  shared_public_key{};
  };
  SecretKeyReq RequestSecretKey(MuddleAddress const &address);

  void SubmitSignatureShare(uint64_t round, crypto::bls::Id const &id,
                            crypto::bls::PublicKey const &public_key,
                            crypto::bls::Signature const &signature);
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

  void ResetCabinet(CabinetMembers cabinet, std::size_t threshold)
  {
    FETCH_LOCK(cabinet_lock_);
    current_cabinet_   = std::move(cabinet);
    current_threshold_ = threshold;
  }
  /// @}

  // Operators
  DkgService &operator=(DkgService const &) = delete;
  DkgService &operator=(DkgService &&) = delete;

private:
  using Address         = ledger::Address;
  using SubscriptionPtr = std::shared_ptr<muddle::Subscription>;
  using AddressMapping  = core::Mapping<MuddleAddress, Address>;
  using EntropyHistory  = std::unordered_map<uint64_t, uint64_t>;
  using CabinetIds      = std::unordered_map<MuddleAddress, crypto::bls::Id>;
  using CabinetKeys     = std::unordered_map<MuddleAddress, crypto::bls::PrivateKey>;
  using StateMachine    = core::StateMachine<State>;
  using StateMachinePtr = std::shared_ptr<StateMachine>;
  using RpcProtocolPtr  = std::unique_ptr<DkgRpcProtocol>;
  using Promise         = service::Promise;
  using RMutex          = std::recursive_mutex;
  using Mutex           = std::mutex;
  using SignaturePtr    = std::unique_ptr<crypto::bls::Signature>;
  using SignatureMap    = std::map<uint64_t, SignaturePtr>;
  using RoundMap        = std::map<uint64_t, RoundPtr>;
  using PrivateKey      = crypto::bls::PrivateKey;
  using PublicKey       = crypto::bls::PublicKey;
  using PublicKeyList   = crypto::bls::PublicKeyList;

  struct Submission
  {
    uint64_t               round;
    crypto::bls::Id        id;
    crypto::bls::PublicKey public_key;
    crypto::bls::Signature signature;
  };

  using SubmissionList = std::deque<Submission>;

  /// @name State Handlers
  /// @{
  State OnBuildAeonKeysState();
  State OnRequestSecretKeyState();
  State OnWaitForSecretKeyState();
  State OnBroadcastSignatureState();
  State OnCollectSignaturesState();
  State OnCompleteState();
  /// @}

  /// @name Utils
  /// @{
  bool     BuildAeonKeys();
  bool     GetSignaturePayload(uint64_t round, ConstByteArray &payload);
  RoundPtr LookupRound(uint64_t round, bool create = false);
  /// @}

  ConstByteArray const  address_;         ///< Our muddle address
  crypto::bls::Id const id_;              ///< Our BLS ID (derived from the muddle address)
  ConstByteArray const  dealer_address_;  ///< The address of the dealer
  bool const            is_dealer_;       ///< Flag to signal if we are the dealer
  Endpoint &            endpoint_;        ///< The muddle endpoint to communicate on
  muddle::rpc::Server   rpc_server_;      ///< The services' RPC server
  muddle::rpc::Client   rpc_client_;      ///< The services' RPC client
  RpcProtocolPtr        rpc_proto_;       ///< The services RPC protocol
  StateMachinePtr       state_machine_;   ///< The service state machine

  /// @name State Machine Data
  /// @{
  Promise    pending_promise_;          ///< The cached pending promise
  PrivateKey aeon_secret_share_{};      ///< The current secret share for the aeon
  PublicKey  aeon_share_public_key_{};  /// < The shared public key for the aeon
  PublicKey  aeon_public_key_{};        ///< The public key for our secret share
  /// @}

  /// @name Cabinet / Aeon Data
  /// @{
  mutable RMutex cabinet_lock_{};        // Priority 1.
  std::size_t    current_threshold_{1};  ///< The current threshold for the aeon
  CabinetMembers current_cabinet_{};     ///< The set of muddle addresses of the cabinet
  /// @}

  /// @name Dealer Specific Data
  /// @{
  mutable RMutex dealer_lock_;                // Priority 2.
  PublicKey      shared_public_key_;          ///< The shared public key for the aeon
  CabinetKeys    current_cabinet_secrets_{};  ///< The map of address to secrets
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

template <typename T>
void Serialize(T &stream, DkgService::SecretKeyReq const &req)
{
  stream << req.success;

  if (req.success)
  {
    stream << req.secret_share << req.shared_public_key;
  }
}

template <typename T>
void Deserialize(T &stream, DkgService::SecretKeyReq &req)
{
  stream >> req.success;

  if (req.success)
  {
    stream >> req.secret_share >> req.shared_public_key;
  }
}

}  // namespace dkg
}  // namespace fetch
