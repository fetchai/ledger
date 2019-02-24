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

#include "ledger/protocols/main_chain_rpc_service.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/logger.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/counter.hpp"
#include "core/service_ids.hpp"
#include "ledger/chain/block_coordinator.hpp"
#include "metrics/metrics.hpp"
#include "network/muddle/packet.hpp"

using fetch::muddle::Packet;
using fetch::byte_array::ToBase64;

using BlockSerializer        = fetch::serializers::ByteArrayBuffer;
using BlockSerializerCounter = fetch::serializers::SizeCounter<BlockSerializer>;

namespace fetch {
namespace ledger {

MainChainRpcService::MainChainRpcService(MuddleEndpoint &endpoint, MainChain &chain,
                                         TrustSystem &trust, BlockCoordinator &block_coordinator)
  : muddle::rpc::Server(endpoint, SERVICE_MAIN_CHAIN, CHANNEL_RPC)
  , endpoint_(endpoint)
  , chain_(chain)
  , trust_(trust)
  , block_coordinator_(block_coordinator)
  , block_subscription_(endpoint.Subscribe(SERVICE_MAIN_CHAIN, CHANNEL_BLOCKS))
  , main_chain_protocol_(chain_)
  , rpc_client_("R:MChain", endpoint, Address{}, SERVICE_MAIN_CHAIN, CHANNEL_RPC)
  , state_machine_{std::make_shared<StateMachine>("MainChain", State::SYNCHRONISING)}
{
  // register the main chain protocol
  Add(RPC_MAIN_CHAIN, &main_chain_protocol_);

  // configure the state machine
  // clang-format off
  state_machine_->RegisterHandler(State::SYNCHRONISING,        this, &MainChainRpcService::OnSynchronising);
  state_machine_->RegisterHandler(State::WAITING_FOR_RESPONSE, this, &MainChainRpcService::OnWaitingForResponse);
  state_machine_->RegisterHandler(State::SYNCHRONISED,         this, &MainChainRpcService::OnSynchronised);
  // clang-format on

  state_machine_->OnStateChange([](State current, State previous) {
    FETCH_LOG_INFO(LOGGING_NAME, "Changed state: ", ToString(previous), " -> ", ToString(current));
  });

  // set the main chain
  block_subscription_->SetMessageHandler([this](Address const &from, uint16_t, uint16_t, uint16_t,
                                                Packet::Payload const &payload,
                                                Address                transmitter) {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Triggering new block handler");

    BlockSerializer serialiser(payload);

    // deserialize the block
    Block block;
    serialiser >> block;

    // recalculate the block hash
    block.UpdateDigest();

    // dispatch the event
    OnNewBlock(from, block, transmitter);
  });
}

MainChainRpcService::~MainChainRpcService()
{
  state_machine_->Reset();
  state_machine_.reset();
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

void MainChainRpcService::OnNewBlock(Address const &from, Block &block, Address const &transmitter)
{
#ifdef FETCH_LOG_INFO_ENABLED
  // count how many transactions are present in the block
  for (auto const &slice : block.body.slices)
  {
    for (auto const &tx : slice)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Recv Ref TX: ", ToBase64(tx.transaction_hash)," (", tx.contract_name, ')');
    }
  }
#endif // FETCH_LOG_INFO_ENABLED

  FETCH_LOG_INFO(LOGGING_NAME, "Recv Block: ", ToBase64(block.body.hash),
                 " (from peer: ", ToBase64(from), " num txs: ", block.GetTransactionCount(), ")");

  FETCH_METRIC_BLOCK_RECEIVED(block.body.hash);

  if (block.proof())
  {
    trust_.AddFeedback(transmitter, p2p::TrustSubject::BLOCK, p2p::TrustQuality::NEW_INFORMATION);

    FETCH_METRIC_BLOCK_RECEIVED(block.body.hash);

    // add the new block to the chain
    auto const status = chain_.AddBlock(block);

    switch (status)
    {
    case BlockStatus::ADDED:
      FETCH_LOG_INFO(LOGGING_NAME, "Added new block: ", ToBase64(block.body.hash));
      break;
    case BlockStatus::LOOSE:
      FETCH_LOG_INFO(LOGGING_NAME, "Added loose block: ", ToBase64(block.body.hash));
      break;
    case BlockStatus::DUPLICATE:
      FETCH_LOG_INFO(LOGGING_NAME, "Duplicate block: ", ToBase64(block.body.hash));
      break;
    case BlockStatus::INVALID:
      FETCH_LOG_INFO(LOGGING_NAME, "Attempted to add invalid block: ", ToBase64(block.body.hash));
      break;
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Invalid Block Recv: ", ToBase64(block.body.hash),
                   " (from: ", ToBase64(from), ")");
  }
}

char const *MainChainRpcService::ToString(State state)
{
  char const *text = "unknown";

  switch (state)
  {
  case State::SYNCHRONISING:
    text = "Synchronising";
    break;
  case State::WAITING_FOR_RESPONSE:
    text = "Waiting for Resposne";
    break;
  case State::SYNCHRONISED:
    text = "Synchronised";
    break;
  }

  return text;
}

MainChainRpcService::Address MainChainRpcService::GetRandomTrustedPeer() const
{
  static random::LinearCongruentialGenerator rng;

  Address address{};

  auto const direct_peers = endpoint_.GetDirectlyConnectedPeers();

  if (!direct_peers.empty())
  {
    // generate a random peer index
    std::size_t const index = rng() % direct_peers.size();

    // select the address
    address = direct_peers[index];
  }

  return address;
}

void MainChainRpcService::HandleChainResponse(Address const &address, BlockList block_list)
{
  for (auto it = block_list.rbegin(), end = block_list.rend(); it != end; ++it)
  {
    // skip the geneis block
    if (it->body.previous_hash == GENESIS_DIGEST)
    {
      continue;
    }

    // recompute the digest
    it->UpdateDigest();

    // add the block
    if (it->proof())
    {
      auto const status = chain_.AddBlock(*it);

      switch (status)
      {
      case BlockStatus::ADDED:
        FETCH_LOG_INFO(LOGGING_NAME, "Synced new block: ", ToBase64(it->body.hash), " from: muddle://", ToBase64(address));
        break;
      case BlockStatus::LOOSE:
        FETCH_LOG_INFO(LOGGING_NAME, "Synced loose block: ", ToBase64(it->body.hash), " from: muddle://", ToBase64(address));
        break;
      case BlockStatus::DUPLICATE:
        FETCH_LOG_INFO(LOGGING_NAME, "Synced duplicate block: ", ToBase64(it->body.hash), " from: muddle://", ToBase64(address));
        break;
      case BlockStatus::INVALID:
        FETCH_LOG_WARN(LOGGING_NAME, "Synced invalid block: ", ToBase64(it->body.hash), " from: muddle://", ToBase64(address));
        break;
      }
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Synced bad proof block: ", ToBase64(it->body.hash), " from: muddle://", ToBase64(address));
    }
  }
}

MainChainRpcService::State MainChainRpcService::OnSynchronising()
{
  State next_state{State::SYNCHRONISED};

  // get the next missing block
  auto const missing_blocks = chain_.GetMissingBlockHashes(1u);

  if (!missing_blocks.empty())
  {
    current_peer_address_  = GetRandomTrustedPeer();
    current_missing_block_ = missing_blocks[0];

    FETCH_LOG_INFO(LOGGING_NAME, "Requesting chain from muddle://", ToBase64(current_peer_address_), " for block ", ToBase64(current_missing_block_));

    // make the RPC call to the block source with a request for the chain
    current_request_ = rpc_client_.CallSpecificAddress(current_peer_address_,
                                                       RPC_MAIN_CHAIN,
                                                       MainChainProtocol::COMMON_SUB_CHAIN,
                                                       current_missing_block_,
                                                       chain_.GetHeaviestBlockHash(),
                                                       uint64_t{1000});

    next_state = State::WAITING_FOR_RESPONSE;
  }

  return next_state;
}

MainChainRpcService::State MainChainRpcService::OnWaitingForResponse()
{
  using service::PromiseState;

  State next_state{State::WAITING_FOR_RESPONSE};

  if (!current_request_)
  {
    next_state = State::SYNCHRONISED;
  }
  else
  {
    // determine the status of the request that is in flight
    auto const status = current_request_->GetState();

    if (PromiseState::WAITING != status)
    {
      if (PromiseState::SUCCESS == status)
      {
        // the request was successful, simply hand off the blocks to be added to the chain
        HandleChainResponse(current_peer_address_, current_request_->As<BlockList>());
      }
      else
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Chain request to: ", ToBase64(current_peer_address_), " failed. Reason: ", service::ToString(status));
      }

      // clear the state
      current_peer_address_  = Address{};
      current_missing_block_ = BlockHash{};
      next_state             = State::SYNCHRONISED;
    }
  }

  return next_state;
}

MainChainRpcService::State MainChainRpcService::OnSynchronised(State current, State previous)
{
  State next_state{State::SYNCHRONISED};

  if (chain_.HasMissingBlocks())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Synchronisation Lost");

    next_state = State::SYNCHRONISING;
  }
  else if (previous != State::SYNCHRONISED)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Synchronised");
  }

  return next_state;
}

}  // namespace ledger
}  // namespace fetch
