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

#include "core/future_timepoint.hpp"
#include "core/mutex.hpp"
#include "core/random/lcg.hpp"
#include "core/state_machine.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/protocols/main_chain_rpc_protocol.hpp"
#include "network/generics/backgrounded_work.hpp"
#include "network/generics/has_worker_thread.hpp"
#include "network/generics/requesting_queue.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/muddle/subscription.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"

#include <memory>

namespace fetch {
namespace ledger {

class BlockCoordinator;
class MainChain;
class MainChainSyncWorker;

class MainChainRpcService : public muddle::rpc::Server,
                            public std::enable_shared_from_this<MainChainRpcService>
{
public:
  enum class State
  {
    REQUEST_HEAVIEST_CHAIN,
    WAIT_FOR_HEAVIEST_CHAIN,
    SYNCHRONISING,
    WAITING_FOR_RESPONSE,
    SYNCHRONISED,
  };

  using MuddleEndpoint  = muddle::MuddleEndpoint;
  using MainChain       = ledger::MainChain;
  using Subscription    = muddle::Subscription;
  using SubscriptionPtr = std::shared_ptr<Subscription>;
  using Address         = muddle::Packet::Address;
  using Block           = ledger::Block;
  using BlockHash       = Block::Digest;
  using Promise         = service::Promise;
  using RpcClient       = muddle::rpc::Client;
  using TrustSystem     = p2p::P2PTrustInterface<Address>;
  using FutureTimepoint = core::FutureTimepoint;

  using Worker                    = MainChainSyncWorker;
  using WorkerPtr                 = std::shared_ptr<Worker>;
  using BackgroundedWork          = network::BackgroundedWork<Worker>;
  using BackgroundedWorkThread    = network::HasWorkerThread<BackgroundedWork>;
  using BackgroundedWorkThreadPtr = std::shared_ptr<BackgroundedWorkThread>;

  static constexpr char const *LOGGING_NAME = "MainChainRpc";

  enum class Mode
  {
    STANDALONE,       ///< Single instance network
    PRIVATE_NETWORK,  ///< Network between a series of private peers
    PUBLIC_NETWORK,   ///< Network restricted to public miners
  };

  // Construction / Destruction
  MainChainRpcService(MuddleEndpoint &endpoint, MainChain &chain, TrustSystem &trust, Mode mode);
  MainChainRpcService(MainChainRpcService const &) = delete;
  MainChainRpcService(MainChainRpcService &&)      = delete;
  ~MainChainRpcService() override                  = default;

  core::WeakRunnable GetWeakRunnable()
  {
    return state_machine_;
  }

  std::weak_ptr<core::StateMachineInterface> GetWeakStateMachine()
  {
    return state_machine_;
  }

  void BroadcastBlock(Block const &block);

  State state() const
  {
    return state_machine_->state();
  }

  bool IsSynced() const
  {
    return State::SYNCHRONISED == state_machine_->state();
  }

  // Operators
  MainChainRpcService &operator=(MainChainRpcService const &) = delete;
  MainChainRpcService &operator=(MainChainRpcService &&) = delete;

private:
  using BlockList       = fetch::ledger::MainChainProtocol::Blocks;
  using StateMachine    = core::StateMachine<State>;
  using StateMachinePtr = std::shared_ptr<StateMachine>;

  /// @name Subscription Handlers
  /// @{
  void OnNewBlock(Address const &from, Block &block, Address const &transmitter);
  /// @}

  /// @name Utilities
  /// @{
  static char const *ToString(State state);
  Address            GetRandomTrustedPeer() const;
  void               HandleChainResponse(Address const &peer, BlockList block_list);
  bool               IsBlockValid(Block &block) const;
  /// @}

  /// @name State Machine Handlers
  /// @{
  State OnRequestHeaviestChain();
  State OnWaitForHeaviestChain();
  State OnSynchronising();
  State OnWaitingForResponse();
  State OnSynchronised(State current, State previous);
  /// @}

  /// @name System Components
  /// @{
  Mode const      mode_;
  MuddleEndpoint &endpoint_;
  MainChain &     chain_;
  TrustSystem &   trust_;
  /// @}

  /// @name RPC Server
  /// @{
  SubscriptionPtr   block_subscription_;
  MainChainProtocol main_chain_protocol_;
  /// @}

  /// @name State Machine Data
  /// @{
  RpcClient       rpc_client_;
  StateMachinePtr state_machine_;
  Address         current_peer_address_;
  BlockHash       current_missing_block_;
  Promise         current_request_;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
