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
#include "dkg/dkg_rpc_serializers.hpp"
#include "dkg/round.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/consensus/entropy_generator_interface.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace fetch {
namespace muddle {

class MuddleEndpoint;
class Subscription;

}  // namespace muddle

namespace dkg {

class DkgService : public ledger::EntropyGeneratorInterface
{
public:
  enum class State
  {
    REGISTER,
    WAIT_FOR_REGISTRATION,
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
  explicit DkgService(Endpoint &endpoint, ConstByteArray address, ConstByteArray beacon_address,
                      std::size_t key_lifetime);
  DkgService(DkgService const &) = delete;
  DkgService(DkgService &&)      = delete;
  ~DkgService() override         = default;

  /// @name External Events
  /// @{
  bool RegisterCabinetMember(MuddleAddress const &address, crypto::bls::Id const &id);

  struct SecretKeyReq
  {
    bool                       success{false};
    crypto::bls::PrivateKey    private_key{};
    crypto::bls::PublicKeyList public_keys{};
    crypto::bls::PublicKey     global_public_key{};
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
  using PublicKeyList   = crypto::bls::PublicKeyList;

  /// @name State Handlers
  /// @{
  State OnRegisterState();
  State OnWaitForRegistrationState();
  State OnBuildAeonKeysState();
  State OnRequestSecretKeyState();
  State OnWaitForSecretKeyState();
  State OnBroadcastSignatureState();
  State OnCollectSignaturesState();
  State OnCompleteState();
  /// @}

  /// @name Utils
  /// @{
  bool           CanBuildAeonKeys() const;
  bool           BuildAeonKeys();
  ConstByteArray GenerateMessage(uint64_t round);
  RoundPtr       LookupRound(uint64_t round, bool create = false);
  /// @}

  ConstByteArray const  address_;
  crypto::bls::Id const id_;
  ConstByteArray const  beacon_address_;
  bool const            is_dealer_;
  Endpoint &            endpoint_;
  muddle::rpc::Server   rpc_server_;
  muddle::rpc::Client   rpc_client_;
  RpcProtocolPtr        rpc_proto_;
  StateMachinePtr       state_machine_;

  /// @name State Machine Data
  /// @{
  Promise                 pending_promise_;
  crypto::bls::PrivateKey sig_private_key_{};
  crypto::bls::PublicKey  sig_public_key_{};  // global

  /// @}

  /// @name Cabinet / Aeon Data
  /// @{
  mutable RMutex cabinet_lock_{};  // Priority 1.
  std::size_t    current_threshold_{1};
  CabinetMembers current_cabinet_{};
  /// @}

  /// @name Dealer Specific Data
  /// @{
  mutable RMutex               dealer_lock_;  // Priority 2.
  CabinetIds                   current_cabinet_ids_{};
  crypto::bls::PublicKey       global_pk_;
  std::vector<crypto::bls::Id> current_cabinet_id_vec_{};
  PublicKeyList                current_cabinet_public_keys_{};
  CabinetKeys                  current_cabinet_secrets_{};
  /// @}

  /// @name Round Data
  /// @{
  mutable RMutex        round_lock_{};  // Priority 3.
  std::atomic<uint64_t> current_iteration_{0};
  std::atomic<uint64_t> requesting_iteration_{0};
  RoundMap              rounds_{};
  /// @}
};

template <typename T>
void Serialize(T &stream, DkgService::SecretKeyReq const &req)
{
  stream << req.success;

  if (req.success)
  {
    stream << req.private_key << req.public_keys;
  }
}

template <typename T>
void Deserialize(T &stream, DkgService::SecretKeyReq &req)
{
  stream >> req.success;

  if (req.success)
  {
    stream >> req.private_key >> req.public_keys;
  }
}

}  // namespace dkg
}  // namespace fetch
