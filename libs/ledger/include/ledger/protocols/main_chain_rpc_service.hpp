#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/mutex.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/protocols/main_chain_rpc_protocol.hpp"
#include "network/generics/requesting_queue.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/muddle/subscription.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"

#include <memory>

namespace fetch {
namespace chain {
class MainChain;
}
namespace ledger {

class MainChainRpcService : public muddle::rpc::Server,
                            public std::enable_shared_from_this<MainChainRpcService>
{
public:
  using MuddleEndpoint  = muddle::MuddleEndpoint;
  using MainChain       = chain::MainChain;
  using Subscription    = muddle::Subscription;
  using SubscriptionPtr = std::shared_ptr<Subscription>;
  using Address         = muddle::Packet::Address;
  using Block           = chain::MainChain::BlockType;
  using Promise         = service::Promise;
  using RpcClient       = muddle::rpc::Client;
  using TrustSystem     = p2p::P2PTrustInterface<Address>;

  MainChainRpcService(MuddleEndpoint &endpoint, MainChain &chain, TrustSystem &trust);

  void BroadcastBlock(Block const &block);

private:
  using BlockList     = fetch::ledger::MainChainProtocol::BlockList;
  using ChainRequests = network::RequestingQueueOf<Address, BlockList>;

  void OnNewBlock(Address const &from, Block &block);

  bool RequestHeaviestChainFromPeer(Address const &from);

  MuddleEndpoint &endpoint_;
  MainChain &     chain_;
  TrustSystem &   trust_;
  SubscriptionPtr block_subscription_;

  MainChainProtocol main_chain_protocol_;

  Mutex     main_chain_rpc_client_lock_{__LINE__, __FILE__};
  RpcClient main_chain_rpc_client_;

  ChainRequests chain_requests_;
};

}  // namespace ledger
}  // namespace fetch
