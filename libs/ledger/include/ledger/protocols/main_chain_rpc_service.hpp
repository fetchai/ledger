#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "ledger/consensus/consensus_interface.hpp"
#include "ledger/protocols/main_chain_rpc_protocol.hpp"
#include "moment/deadline_timer.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"
#include "network/generics/backgrounded_work.hpp"
#include "network/generics/has_worker_thread.hpp"
#include "network/generics/requesting_queue.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"
#include "telemetry/telemetry.hpp"

#include <limits>
#include <memory>

namespace fetch {
namespace ledger {

class BlockCoordinator;
class MainChain;
class MainChainRpcClientInterface;
class MainChainSyncWorker;

/**
 * The main chain rpc service ensures that nodes synchronise the main chain. Blocks are broadcast
 * around and nodes will attempt to determine the heaviest chain of their peers and specifically
 * request them. Peers are guarded by the main chain limiting request sizes.
 *
 *                                       ┌───────────────────┐
 *                                       │                   │
 *                            ┌───────── │   Synchronising   │────────┐
 *                            │          │                   │        │
 *                            │          └───────────────────┘        │
 *                            │                    ▲                  │
 *                            ▼                    │                  ▼
 *                  ┌───────────────────┐          │        ┌───────────────────┐
 *                  │  Start Sync with  │          │        │                   │
 *                  │       Peer        │          ├────────│   Synchronised    │
 *                  │                   │          │        │                   │
 *                  └───────────────────┘          │        └───────────────────┘
 *                            │                    │
 *                            │                    │
 *                            ▼                    │
 *                  ┌───────────────────┐          │
 *                  │                   │          │
 *           ┌─────▶│Request Next Blocks│          │
 *           │      │                   │          │
 *           │      └───────────────────┘          │
 *           │                │                    │
 *           │                │                    │
 *           │                ▼                    │
 *           │      ┌───────────────────┐          │
 *           │      │   Wait for Next   │          │
 *           └──────│      Blocks       │          │
 *                  │                   │          │
 *                  └───────────────────┘          │
 *                            │                    │
 *                            │                    │
 *                            ▼                    │
 *                  ┌───────────────────┐          │
 *                  │Complete Sync with │          │
 *                  │       Peer        │          │
 *                  │                   │          │
 *                  └───────────────────┘          │
 *                            │                    │
 *                            │                    │
 *                            └────────────────────┘
 */
class MainChainRpcService : public muddle::rpc::Server,
                            public std::enable_shared_from_this<MainChainRpcService>
{
public:
  enum class State
  {
    SYNCHRONISING,
    SYNCHRONISED,
    START_SYNC_WITH_PEER,
    REQUEST_NEXT_BLOCKS,
    WAIT_FOR_NEXT_BLOCKS,
    COMPLETE_SYNC_WITH_PEER,
  };

  using MuddleEndpoint  = muddle::MuddleEndpoint;
  using MainChain       = ledger::MainChain;
  using Subscription    = muddle::Subscription;
  using SubscriptionPtr = std::shared_ptr<Subscription>;
  using Address         = muddle::Packet::Address;
  using Block           = ledger::Block;
  using BlockHash       = Digest;
  using Promise         = service::Promise;
  using RpcClient       = MainChainRpcClientInterface;
  using TrustSystem     = p2p::P2PTrustInterface<Address>;
  using FutureTimepoint = core::FutureTimepoint;
  using ConsensusPtr    = std::shared_ptr<ConsensusInterface>;

  static constexpr char const *LOGGING_NAME            = "MainChainRpc";
  static constexpr uint64_t    PERIODIC_RESYNC_SECONDS = 20;

  enum class Mode
  {
    STANDALONE,       ///< Single instance network
    PRIVATE_NETWORK,  ///< Network between a series of private peers
    PUBLIC_NETWORK,   ///< Network restricted to public miners
  };

  // Construction / Destruction
  MainChainRpcService(MuddleEndpoint &endpoint, MainChainRpcClientInterface &rpc_client,
                      MainChain &chain, TrustSystem &trust, ConsensusPtr consensus);
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

  /// @name Subscription Handlers
  /// @{
  void OnNewBlock(Address const &from, Block &block, Address const &transmitter);
  /// @}

  // Operators
  MainChainRpcService &operator=(MainChainRpcService const &) = delete;
  MainChainRpcService &operator=(MainChainRpcService &&) = delete;

private:
  using BlockList       = fetch::ledger::MainChainProtocol::Blocks;
  using StateMachine    = core::StateMachine<State>;
  using StateMachinePtr = std::shared_ptr<StateMachine>;
  using BlockPtr        = MainChain::BlockPtr;
  using DeadlineTimer   = fetch::moment::DeadlineTimer;

  /// @name Utilities
  /// @{
  Address GetRandomTrustedPeer() const;

  void HandleChainResponse(Address const &address, BlockList blocks);
  template <class Begin, class End>
  void HandleChainResponse(Address const &address, Begin begin, End end);
  /// @}

  /// @name State Machine Handlers
  /// @{
  State OnSynchronising();
  State OnSynchronised(State current, State previous);
  State OnStartSyncWithPeer();
  State OnRequestNextSetOfBlocks();
  State OnWaitForBlocks();
  State OnCompleteSyncWithPeer();

  bool ValidBlock(Block const &block, char const *action) const;
  /// @}

  /// @name System Components
  /// @{
  MuddleEndpoint &endpoint_;
  MainChain &     chain_;
  TrustSystem &   trust_;
  /// @}

  /// @name Block Validation
  /// @{
  ConsensusPtr consensus_;
  /// @}

  /// @name RPC Server
  /// @{
  SubscriptionPtr   block_subscription_;
  MainChainProtocol main_chain_protocol_;
  /// @}

  /// @name State Machine Data
  /// @{
  RpcClient &     rpc_client_;
  StateMachinePtr state_machine_;

  Address       current_peer_address_;
  Promise       current_request_;
  BlockPtr      block_resolving_;
  DeadlineTimer resync_interval_{"MC_RPC:main"};
  std::size_t   consecutive_failures_{0};

  BlockHash             current_missing_block_;
  std::atomic<uint16_t> loose_blocks_seen_{0};
  /// @}

  /// @name Telemetry
  /// @{
  telemetry::CounterPtr         recv_block_count_;
  telemetry::CounterPtr         recv_block_valid_count_;
  telemetry::CounterPtr         recv_block_loose_count_;
  telemetry::CounterPtr         recv_block_duplicate_count_;
  telemetry::CounterPtr         recv_block_invalid_count_;
  telemetry::CounterPtr         recv_block_dirty_count_;
  telemetry::CounterPtr         state_synchronising_;
  telemetry::CounterPtr         state_synchronised_;
  telemetry::CounterPtr         state_start_sync_with_peer_;
  telemetry::CounterPtr         state_request_next_blocks_;
  telemetry::CounterPtr         state_wait_for_next_blocks_;
  telemetry::CounterPtr         state_complete_sync_with_peer_;
  telemetry::GaugePtr<uint32_t> state_current_;
  telemetry::HistogramPtr       new_block_duration_;
  /// @}
};

constexpr char const *ToString(MainChainRpcService::State state) noexcept
{
  switch (state)
  {
  case MainChainRpcService::State::SYNCHRONISING:
    return "Synchronising";
  case MainChainRpcService::State::SYNCHRONISED:
    return "Synchronised";
  case MainChainRpcService::State::START_SYNC_WITH_PEER:
    return "Starting Sync with Peer";
  case MainChainRpcService::State::REQUEST_NEXT_BLOCKS:
    return "Requesting Blocks";
  case MainChainRpcService::State::WAIT_FOR_NEXT_BLOCKS:
    return "Waiting for Blocks";
  case MainChainRpcService::State::COMPLETE_SYNC_WITH_PEER:
    return "Completed Sync with Peer";
  }

  return "unknown";
}

}  // namespace ledger
}  // namespace fetch
