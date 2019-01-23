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

#include <memory>

#include "core/mutex.hpp"
#include "ledger/chain/block_coordinator.hpp"            // for BlockCoordin...
#include "ledger/chain/main_chain.hpp"                   // for MainChain
#include "ledger/protocols/main_chain_rpc_protocol.hpp"  // for MainChainPro...
#include "network/generics/backgrounded_work.hpp"        // for Backgrounded...
#include "network/generics/future_timepoint.hpp"         // for FutureTimepoint
#include "network/generics/has_worker_thread.hpp"        // for HasWorkerThread
#include "network/generics/requesting_queue.hpp"
#include "network/muddle/muddle_endpoint.hpp"  // for MuddleEndpoint
#include "network/muddle/rpc/client.hpp"       // for Client
#include "network/muddle/rpc/server.hpp"       // for Server
#include "network/muddle/subscription.hpp"     // for Subscription
#include "network/p2pservice/p2ptrust_interface.hpp"

namespace fetch {
namespace chain {
class MainChain;
class BlockCoordinator;
}  // namespace chain
namespace ledger {

class MainChainSyncWorker;

class MainChainRpcService : public muddle::rpc::Server,
                            public std::enable_shared_from_this<MainChainRpcService>
{
public:
  friend class MainChainSyncWorker;
  using MuddleEndpoint   = muddle::MuddleEndpoint;
  using MainChain        = chain::MainChain;
  using BlockCoordinator = chain::BlockCoordinator;
  using Subscription     = muddle::Subscription;
  using SubscriptionPtr  = std::shared_ptr<Subscription>;
  using Address          = muddle::Packet::Address;
  using Block            = chain::MainChain::BlockType;
  using BlockHash        = chain::MainChain::BlockHash;
  using Promise          = service::Promise;
  using RpcClient        = muddle::rpc::Client;
  using TrustSystem      = p2p::P2PTrustInterface<Address>;
  using FutureTimepoint  = network::FutureTimepoint;

  using Worker                    = MainChainSyncWorker;
  using WorkerPtr                 = std::shared_ptr<Worker>;
  using BackgroundedWork          = network::BackgroundedWork<Worker>;
  using BackgroundedWorkThread    = network::HasWorkerThread<BackgroundedWork>;
  using BackgroundedWorkThreadPtr = std::shared_ptr<BackgroundedWorkThread>;

  MainChainRpcService(MuddleEndpoint &endpoint, MainChain &chain, TrustSystem &trust,
                      BlockCoordinator &block_coordinator);

  void BroadcastBlock(Block const &block);

private:
  static constexpr std::size_t BLOCK_CATCHUP_STEP_SIZE = 30;

  using BlockList     = fetch::ledger::MainChainProtocol::BlockList;
  using ChainRequests = network::RequestingQueueOf<Address, BlockList>;

  void OnNewBlock(Address const &from, Block &block, Address const &transmitter);

  bool RequestHeaviestChainFromPeer(Address const &from);

  void AddLooseBlock(const BlockHash &hash, const Address &address);
  void ServiceLooseBlocks();
  void RequestedChainArrived(Address const &peer, BlockList block_list);

  MuddleEndpoint &  endpoint_;
  MainChain &       chain_;
  TrustSystem &     trust_;
  BlockCoordinator &block_coordinator_;
  SubscriptionPtr   block_subscription_;

  MainChainProtocol main_chain_protocol_;

  Mutex     main_chain_rpc_client_lock_{__LINE__, __FILE__};
  RpcClient main_chain_rpc_client_;

  BackgroundedWork          bg_work_;
  BackgroundedWorkThreadPtr workthread_;

  Address         last_good_address_;
  FutureTimepoint next_loose_tips_check_;
};

}  // namespace ledger
}  // namespace fetch
