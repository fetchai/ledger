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

#include "dkg/dkg_rpc_serializers.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/state_machine.hpp"
#include "core/containers/mapping.hpp"
#include "core/mutex.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/bls_base.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/consensus/entropy_generator_interface.hpp"
#include "dkg/dkg_rpc_protocol.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/muddle/rpc/client.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace fetch {
namespace muddle {

class MuddleEndpoint;
class Subscription;

} // namespace network

namespace dkg {

class SecretKeyMsg;
class ContributionMsg;

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
  using CabinetMembers  = std::unordered_set<MuddleAddress>;

  // Construction / Destruction
  explicit DkgService(Endpoint &endpoint, ConstByteArray address, ConstByteArray beacon_address,
                      std::size_t key_lifetime);
  DkgService(DkgService const &) = delete;
  DkgService(DkgService &&) = delete;
  ~DkgService() override = default;

  /// @name External Events
  /// @{
  bool RegisterCabinetMember(MuddleAddress const &address, crypto::bls::Id const &id);

  struct SecretKeyReq
  {
    bool                       success{false};
    crypto::bls::PrivateKey    private_key{};
    crypto::bls::PublicKeyList public_keys{};
  };
  SecretKeyReq RequestSecretKey(MuddleAddress const &address);

  void SubmitSignatureShare(crypto::bls::Id const &id, crypto::bls::PublicKey  const &public_key, crypto::bls::Signature const &signature);

  void OnNewBlock(uint64_t block_index);
  /// @}

  /// @name Entropy Generator
  /// @{
  Status GenerateEntropy(Digest block_digest, uint64_t block_number, uint64_t &entropy) override;
  /// @}

  std::weak_ptr<core::Runnable> GetWeakRunnable()
  {
    return state_machine_;
  }

  void ResetCabinet(CabinetMembers cabinet)
  {
    FETCH_LOCK(cabinet_lock_);
    current_cabinet_ = std::move(cabinet);
  }

  // Operators
  DkgService &operator=(DkgService const &) = delete;
  DkgService &operator=(DkgService &&) = delete;

private:
  using Address         = ledger::Address;
  using SubscriptionPtr = std::shared_ptr<muddle::Subscription>;
  using AddressMapping  = core::Mapping<MuddleAddress, Address>;

  using CabinetIds      = std::unordered_map<MuddleAddress, crypto::bls::Id>;
  using CabinetKeys     = std::unordered_map<MuddleAddress, crypto::bls::PrivateKey>;
  using StateMachine    = core::StateMachine<State>;
  using StateMachinePtr = std::shared_ptr<StateMachine>;
  using RpcProtocolPtr  = std::unique_ptr<DkgRpcProtocol>;
  using Promise = service::Promise;
  using RMutex = std::recursive_mutex;

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

//  /// @name Network Comms?
//  /// @{
//  void OnContribution(ContributionMsg const &secret_key, ConstByteArray const &from);
//  void OnSecretKey(SecretKeyMsg const &contribution, ConstByteArray const &from);
//  /// @}

  bool CanBuildAeonKeys() const;
  bool BuildAeonKeys();


  ConstByteArray const  address_;
  crypto::bls::Id const id_;
  ConstByteArray const  beacon_address_;
  bool const            is_dealer_;
  Endpoint &            endpoint_;


  muddle::rpc::Server rpc_server_;
  muddle::rpc::Client rpc_client_;
  RpcProtocolPtr      rpc_proto_;


  StateMachinePtr state_machine_;

//
//  SubscriptionPtr       contribution_subscription_;
//  SubscriptionPtr       secret_key_subscription_;

  std::size_t current_threshold_{1};

  /// @name State Spectific
  /// @{
  Promise pending_promise_;
  crypto::bls::PrivateKey     sig_private_key_{};


  /// @}

  /// @name Current Signature
  /// @{
  mutable RMutex              sig_lock_{};
  std::unique_ptr<crypto::bls::Signature> aeon_signature_{};
//  crypto::bls::PublicKeyList  sig_public_keys_{};
  crypto::bls::IdList         sig_ids_{};
  crypto::bls::SignatureList  sig_shares_{};

//  crypto::bls::PrivateKeyList sig_private_shares_{};
  /// @}

  /// @name Beacon / Secret Generation
  /// @{
  mutable RMutex cabinet_lock_;
  CabinetMembers current_cabinet_{
    byte_array::FromBase64("CL3zb8U2KYP+OWmn9wZuHuUIXciSNtTZPjmgB75OHRk8hxgCt+G+sOj1dXVUqzsMcNYAJwV/tScKu8ej1yAuDw=="),
    byte_array::FromBase64("YzYsKxl+EmgLpHLnmrNx2PoNjJSQT4ifFzbQS051iKTcXEWJh+ghGE+Ip8FD6UXBMUlKAm9V+NRzCGo3U/aJiA==")
  };
  CabinetIds current_cabinet_ids_{};
  crypto::bls::PublicKeyList current_cabinet_public_keys_{}; // aeon
  CabinetKeys current_cabinet_secrets_{};
  /// @}

  AddressMapping address_mapper_{}; ///< Muddle <-> Token Address mapping
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

} // namespace dkg
} // namespace fetch
