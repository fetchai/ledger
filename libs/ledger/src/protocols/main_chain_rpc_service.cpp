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

#include "ledger/protocols/main_chain_rpc_service.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/logger.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/counter.hpp"
#include "core/service_ids.hpp"
#include "network/muddle/packet.hpp"

using fetch::muddle::Packet;
using fetch::byte_array::ToBase64;

using BlockSerializer        = fetch::serializers::ByteArrayBuffer;
using BlockSerializerCounter = fetch::serializers::SizeCounter<BlockSerializer>;

namespace fetch {
namespace ledger {

class MainChainSyncWorker
{
  public:
  using BlockHash               = MainChainRpcService::BlockHash;
  using BlockList               = MainChainRpcService::BlockList;
  using Address                 = MainChainRpcService::Address;
  using PromiseState         = fetch::service::PromiseState;
  using Promise              = service::Promise;
  using FutureTimepoint      = network::FutureTimepoint;

  Promise             prom_;
  BlockHash           hash_;
  Address           address_;
  std::shared_ptr<MainChainRpcService> client_;
  FutureTimepoint   timeout_;
  BlockList blocks_;

  static constexpr char const *LOGGING_NAME = "MainChainSyncWorker";

  MainChainSyncWorker(std::shared_ptr<MainChainRpcService> client, BlockHash hash, Address address,
                      std::chrono::milliseconds thetimeout = std::chrono::milliseconds(1000))
    : hash_(std::move(hash))
    , address_(std::move(address))
    , client_(std::move(client))
    , timeout_(thetimeout)
  {

  }

  template<class BlockHash>
  bool Equals(const BlockHash &hash) const
  {
    return hash == hash_;
  }

  PromiseState Work()
  {
    if (!prom_)
    {
      prom_ = client_
        ->main_chain_rpc_client_
        .CallSpecificAddress(address_, RPC_MAIN_CHAIN, MainChainProtocol::CHAIN_PRECEDING, hash_, uint32_t{16});

      FETCH_LOG_INFO(LOGGING_NAME, "CHAIN_PRECEDING : ", ToBase64(hash_));
    }
    auto promise_state = prom_ -> GetState();

    switch (promise_state)
    {
    case PromiseState::TIMEDOUT:
    case PromiseState::FAILED:
      return promise_state;
    case PromiseState::WAITING:
      if (timeout_.IsDue())
      {
        return PromiseState::TIMEDOUT;
      }
      return promise_state;
    case PromiseState::SUCCESS:
      {
        prom_->As(blocks_);
      }
      return promise_state;
    }
  }
};

MainChainRpcService::MainChainRpcService(MuddleEndpoint &endpoint, chain::MainChain &chain,
                                         TrustSystem &trust)
  : muddle::rpc::Server(endpoint, SERVICE_MAIN_CHAIN, CHANNEL_RPC)
  , endpoint_(endpoint)
  , chain_(chain)
  , trust_(trust)
  , block_subscription_(endpoint.Subscribe(SERVICE_MAIN_CHAIN, CHANNEL_BLOCKS))
  , main_chain_protocol_(chain_)
  , main_chain_rpc_client_(endpoint, Address{}, SERVICE_MAIN_CHAIN, CHANNEL_RPC)
{
  // register the main chain protocol
  Add(RPC_MAIN_CHAIN, &main_chain_protocol_);

  // set the main chain
  block_subscription_->SetMessageHandler(
      [this](Address const &from, uint16_t, uint16_t, uint16_t, Packet::Payload const &payload) {
        FETCH_LOG_DEBUG(LOGGING_NAME, "Triggering new block handler");

        BlockSerializer serialiser(payload);

        // deserialize the block
        Block block;
        serialiser >> block;

        // recalculate the block hash
        block.UpdateDigest();

        // dispatch the event
        OnNewBlock(from, block);
      });
}

void MainChainRpcService::BroadcastBlock(MainChainRpcService::Block const &block)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Broadcast Block: ", ToBase64(block.hash()));

  // determine the serialised size of the block
  BlockSerializerCounter counter;
  counter << block;

  // allocate the buffer and serialise the block
  BlockSerializer serializer;
  serializer.Reserve(counter.size());
  serializer << block;

  // broadcast the block to the nodes on the network
  endpoint_.Broadcast(SERVICE_MAIN_CHAIN, CHANNEL_BLOCKS, serializer.data());
}

void MainChainRpcService::OnNewBlock(Address const &from, Block &block)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Recv Block: ", ToBase64(block.hash()),
                 " (from peer: ", ToBase64(from), ')');

  trust_.AddFeedback(from, p2p::TrustSubject::BLOCK, p2p::TrustQuality::NEW_INFORMATION);

  // add the block?
  chain_.AddBlock(block);

  // if we got a block and it is loose then it it probably means that we need to sync the rest of
  // the block tree
  if (block.loose())
  {
    AddLooseBlock(block.hash(), from);
  }
}

void MainChainRpcService::AddLooseBlock(const BlockHash &hash, const Address &address)
{
  if (!workthread_)
  {
    workthread_ =
      std::make_shared<BackgroundedWorkThread>(&bg_work_, [this]() { this->ServiceLooseBlocks(); });
  }

  if (!bg_work_.InFlightP(hash))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Block is loose, requesting longest chain from counter part: ", ToBase64(hash));
    auto worker = std::make_shared<MainChainSyncWorker>(shared_from_this(), hash, address);
    bg_work_.Add(worker);
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Block is loose, query inflight: ", ToBase64(hash));
  }
}

void MainChainRpcService::ServiceLooseBlocks()
{
  auto p = bg_work_.CountPending();

  if (!p)
  {
    // At this point, ask the chain to check it has no tips to query.
  }

  bg_work_.WorkCycle();

  for (auto &successful_worker : bg_work_.Get(MainChainSyncWorker::PromiseState::SUCCESS, 1000))
  {
    if (successful_worker)
    {
      RequestedChainArrived(successful_worker -> address_, successful_worker -> blocks_);
    }
  }
  bg_work_.DiscardFailures();
  bg_work_.DiscardTimeouts();
}



void MainChainRpcService::RequestedChainArrived(Address const &peer, BlockList block_list)
{
  bool newdata = false;
  for (auto it = block_list.rbegin(), end = block_list.rend(); it != end; ++it)
  {
    // recompute the digest
    it->UpdateDigest();

    // add the block
    newdata |= chain_.AddBlock(*it);
  }

  if (newdata)
  {
    trust_.AddFeedback(peer, p2p::TrustSubject::BLOCK, p2p::TrustQuality::NEW_INFORMATION);
  }

  if (newdata && !block_list.empty())
  {
    Block blk;
    if (chain_.Get(block_list.back().hash(), blk))
    {
      blk.UpdateDigest();
      if (blk.loose())
      {
        AddLooseBlock(block_list.back().hash(), peer);
      }
    }
  }

}

  /*
  bool request_made = false;

  FETCH_LOCK(main_chain_rpc_client_lock_);

  // make the request for the heaviest chain
  auto promise = main_chain_rpc_client_.CallSpecificAddress(peer, RPC_MAIN_CHAIN, MainChainProtocol::CHAIN_PRECEDING, hash, uint32_t{16});

  // setup that handlers
  promise->WithHandlers().Then([self = shared_from_this(), promise, peer]() {

      // extract the block list from the promise
      BlockList block_list;
      promise->As(block_list);

      FETCH_LOG_INFO(LOGGING_NAME, "Block Sync: Got ", block_list.size(), " blocks from peer...");

      // iterate through each of the blocks backwards and add them to the chain
      for (auto it = block_list.rbegin(), end = block_list.rend(); it != end; ++it)
      {
        // recompute the digest
        it->UpdateDigest();

        // add the block
        self->chain_.AddBlock(*it);
      }

      auto firsthash = block_list.first().hash();

      // cycle the requesting queue
      self->chain_requests_.Resolve();
      self->chain_requests_.DiscardCompleted();
      self->chain_requests_.DiscardFailures();

      FETCH_LOG_INFO(LOGGING_NAME, "Block Sync: Complete");
    });
}
  */

}  // namespace ledger
}  // namespace fetch
