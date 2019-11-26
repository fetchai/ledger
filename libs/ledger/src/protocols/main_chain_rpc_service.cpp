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

#include "chain/transaction_layout_rpc_serializers.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "crypto/fetch_identity.hpp"
#include "ledger/chain/block_coordinator.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/protocols/main_chain_rpc_service.hpp"
#include "logging/logging.hpp"
#include "muddle/packet.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/registry.hpp"

#include <cstddef>
#include <cstdint>

static const uint32_t MAX_CHAIN_REQUEST_SIZE = 10000;
static const uint64_t MAX_SUB_CHAIN_SIZE     = 1000;

namespace fetch {
namespace ledger {
namespace {

using fetch::muddle::Packet;
using fetch::byte_array::ToBase64;

using BlockSerializer        = fetch::serializers::MsgPackSerializer;
using BlockSerializerCounter = fetch::serializers::SizeCounter;
using PromiseState           = fetch::service::PromiseState;
using State                  = MainChainRpcService::State;
using Mode                   = MainChainRpcService::Mode;

/**
 * Map the initial state of the state machine to the particular mode that is being configured.
 *
 * @param mode The mode for the main chain
 * @return The initial state for the state machine
 */
constexpr State GetInitialState(Mode mode) noexcept
{
  switch (mode)
  {
  case Mode::STANDALONE:
    return State::SYNCHRONISED;
  case Mode::PRIVATE_NETWORK:
  case Mode::PUBLIC_NETWORK:
    break;
  }

  return State::REQUEST_HEAVIEST_CHAIN;
}

}  // namespace

MainChainRpcService::MainChainRpcService(MuddleEndpoint &endpoint, MainChain &chain,
                                         TrustSystem &trust, Mode mode)
  : muddle::rpc::Server(endpoint, SERVICE_MAIN_CHAIN, CHANNEL_RPC)
  , mode_(mode)
  , endpoint_(endpoint)
  , chain_(chain)
  , trust_(trust)
  , block_subscription_(endpoint.Subscribe(SERVICE_MAIN_CHAIN, CHANNEL_BLOCKS))
  , main_chain_protocol_(chain_)
  , rpc_client_("R:MChain", endpoint, SERVICE_MAIN_CHAIN, CHANNEL_RPC)
  , state_machine_{std::make_shared<StateMachine>("MainChain", GetInitialState(mode_),
                                                  [](State state) { return ToString(state); })}
  , recv_block_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_recv_block_total",
        "The number of received blocks from the network")}
  , recv_block_valid_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_recv_block_valid_total",
        "The total number of valid blocks received")}
  , recv_block_loose_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_recv_block_loose_total",
        "The total number of loose blocks received")}
  , recv_block_duplicate_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_recv_block_duplicate_total",
        "The total number of duplicate blocks received from the network")}
  , recv_block_invalid_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_recv_block_invalid_total",
        " The total number of invalid blocks received from the network")}
  , state_request_heaviest_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_state_request_heaviest_total",
        "The number of times in the requested heaviest state")}
  , state_wait_heaviest_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_state_wait_heaviest_total",
        "The number of times in the wait heaviest state")}
  , state_synchronising_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_state_synchronising_total",
        "The number of times in the synchronisiing state")}
  , state_wait_response_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_state_wait_response_total",
        "The number of times in the wait response state")}
  , state_synchronised_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_state_synchronised_total",
        "The number of times in the sychronised state")}
{
  // register the main chain protocol
  Add(RPC_MAIN_CHAIN, &main_chain_protocol_);

  // configure the state machine
  // clang-format off
  state_machine_->RegisterHandler(State::REQUEST_HEAVIEST_CHAIN,  this, &MainChainRpcService::OnRequestHeaviestChain);
  state_machine_->RegisterHandler(State::WAIT_FOR_HEAVIEST_CHAIN, this, &MainChainRpcService::OnWaitForHeaviestChain);
  state_machine_->RegisterHandler(State::SYNCHRONISING,           this, &MainChainRpcService::OnSynchronising);
  state_machine_->RegisterHandler(State::WAITING_FOR_RESPONSE,    this, &MainChainRpcService::OnWaitingForResponse);
  state_machine_->RegisterHandler(State::SYNCHRONISED,            this, &MainChainRpcService::OnSynchronised);
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

void MainChainRpcService::BroadcastBlock(MainChainRpcService::Block const &block)
{
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
  recv_block_count_->increment();

#ifdef FETCH_LOG_DEBUG_ENABLED
  // count how many transactions are present in the block
  for (auto const &slice : block.slices)
  {
    for (auto const &tx : slice)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Recv Ref TX: ", ToBase64(tx.digest()));
    }
  }
#endif  // FETCH_LOG_INFO_ENABLED

  FETCH_LOG_INFO(LOGGING_NAME, "Recv Block: 0x", block.hash.ToHex(),
                 " (from peer: ", ToBase64(from), " num txs: ", block.GetTransactionCount(),
                 " num: ", block.block_number, ")");

  trust_.AddFeedback(transmitter, p2p::TrustSubject::BLOCK, p2p::TrustQuality::NEW_INFORMATION);

  // add the new block to the chain
  auto const status = chain_.AddBlock(block);

  switch (status)
  {
  case BlockStatus::ADDED:
    recv_block_valid_count_->increment();
    FETCH_LOG_INFO(LOGGING_NAME, "Added new block: 0x", block.hash.ToHex());
    break;
  case BlockStatus::LOOSE:
    recv_block_loose_count_->increment();
    FETCH_LOG_INFO(LOGGING_NAME, "Added loose block: 0x", block.hash.ToHex());
    break;
  case BlockStatus::DUPLICATE:
    recv_block_duplicate_count_->increment();
    FETCH_LOG_INFO(LOGGING_NAME, "Duplicate block: 0x", block.hash.ToHex());
    break;
  case BlockStatus::INVALID:
    recv_block_invalid_count_->increment();
    FETCH_LOG_INFO(LOGGING_NAME, "Attempted to add invalid block: 0x", block.hash.ToHex());
    break;
  }
}

MainChainRpcService::Address MainChainRpcService::GetRandomTrustedPeer() const
{
  static random::LinearCongruentialGenerator rng;

  Address address{};

  auto const direct_peers = endpoint_.GetDirectlyConnectedPeers();

  FETCH_LOG_DEBUG(LOGGING_NAME, "Main chain connected peers: ", direct_peers.size());

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
  std::size_t added{0};
  std::size_t loose{0};
  std::size_t duplicate{0};
  std::size_t invalid{0};

  for (auto it = block_list.rbegin(), end = block_list.rend(); it != end; ++it)
  {
    // skip the genesis block
    if (it->IsGenesis())
    {
      continue;
    }

    // recompute the digest
    it->UpdateDigest();

    // add the block
    // TODO(HUT): put consensus check here.
    {
      auto const status = chain_.AddBlock(*it);

      switch (status)
      {
      case BlockStatus::ADDED:
        FETCH_LOG_DEBUG(LOGGING_NAME, "Synced new block: 0x", it->hash.ToHex(), " from: muddle://",
                        ToBase64(address));
        ++added;
        break;
      case BlockStatus::LOOSE:
        FETCH_LOG_DEBUG(LOGGING_NAME, "Synced loose block: 0x", it->hash.ToHex(),
                        " from: muddle://", ToBase64(address));
        ++loose;
        break;
      case BlockStatus::DUPLICATE:
        FETCH_LOG_DEBUG(LOGGING_NAME, "Synced duplicate block: 0x", it->hash.ToHex(),
                        " from: muddle://", ToBase64(address));
        ++duplicate;
        break;
      case BlockStatus::INVALID:
        FETCH_LOG_DEBUG(LOGGING_NAME, "Synced invalid block: 0x", it->hash.ToHex(),
                        " from: muddle://", ToBase64(address));
        ++invalid;
        break;
      }
    }
  }

  if (invalid != 0u)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Synced Summary: Invalid: ", invalid, " Added: ", added,
                   " Loose: ", loose, " Duplicate: ", duplicate, " from: muddle://",
                   ToBase64(address));
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Synced Summary: Added: ", added, " Loose: ", loose,
                   " Duplicate: ", duplicate, " from: muddle://", ToBase64(address));
  }
}

/**
 * Request from a random peer the heaviest chain, starting from the newest block
 * and going backwards. The client is free to return less blocks than requested.
 *
 */
MainChainRpcService::State MainChainRpcService::OnRequestHeaviestChain()
{
  state_request_heaviest_->increment();

  State next_state{State::REQUEST_HEAVIEST_CHAIN};

  auto const peer = GetRandomTrustedPeer();

  if (!peer.empty())
  {
    current_peer_address_ = peer;
    current_request_ =
        rpc_client_.CallSpecificAddress(current_peer_address_, RPC_MAIN_CHAIN,
                                        MainChainProtocol::HEAVIEST_CHAIN, MAX_CHAIN_REQUEST_SIZE);

    next_state = State::WAIT_FOR_HEAVIEST_CHAIN;
  }

  state_machine_->Delay(std::chrono::milliseconds{500});
  return next_state;
}

MainChainRpcService::State MainChainRpcService::OnWaitForHeaviestChain()
{
  state_wait_heaviest_->increment();

  State next_state{State::WAIT_FOR_HEAVIEST_CHAIN};

  if (!current_request_)
  {
    // something went wrong we should attempt to request the chain again
    next_state = State::REQUEST_HEAVIEST_CHAIN;
  }
  else
  {
    // determine the status of the request that is in flight
    auto const status = current_request_->state();

    if (PromiseState::WAITING != status)
    {
      if (PromiseState::SUCCESS == status)
      {
        // the request was successful, simply hand off the blocks to be added to the chain
        HandleChainResponse(current_peer_address_, current_request_->As<BlockList>());

        // now we have completed a request we can start normal synchronisation
        next_state = State::SYNCHRONISING;
      }
      else
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Heaviest chain request to: ", ToBase64(current_peer_address_),
                       " failed. Reason: ", service::ToString(status));

        // since we want to sync at least with one chain before proceeding we restart the state
        // machine back to the requesting
        next_state = State::REQUEST_HEAVIEST_CHAIN;
      }

      // clear the state
      current_peer_address_  = Address{};
      current_missing_block_ = BlockHash{};
    }
  }

  return next_state;
}

MainChainRpcService::State MainChainRpcService::OnSynchronising()
{
  state_synchronising_->increment();

  State next_state{State::SYNCHRONISED};

  // get the next missing block
  auto const missing_blocks = chain_.GetMissingTips();

  if (!missing_blocks.empty())
  {
    current_missing_block_ = *missing_blocks.begin();
    current_peer_address_  = GetRandomTrustedPeer();

    // in the case that we don't trust any one we need to simply wait until we do
    if (current_peer_address_.empty())
    {
      return State::SYNCHRONISING;
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Requesting chain from muddle://", ToBase64(current_peer_address_),
                   " for block ", ToBase64(current_missing_block_));

    // make the RPC call to the block source with a request for the chain
    current_request_ = rpc_client_.CallSpecificAddress(
        current_peer_address_, RPC_MAIN_CHAIN, MainChainProtocol::COMMON_SUB_CHAIN,
        current_missing_block_, chain_.GetHeaviestBlockHash(), MAX_SUB_CHAIN_SIZE);

    next_state = State::WAITING_FOR_RESPONSE;
  }

  return next_state;
}

MainChainRpcService::State MainChainRpcService::OnWaitingForResponse()
{
  state_wait_response_->increment();

  State next_state{State::WAITING_FOR_RESPONSE};

  if (!current_request_)
  {
    next_state = State::SYNCHRONISED;
  }
  else
  {
    // determine the status of the request that is in flight
    auto const status = current_request_->state();

    if (PromiseState::WAITING != status)
    {
      if (PromiseState::SUCCESS == status)
      {
        // the request was successful, simply hand off the blocks to be added to the chain
        HandleChainResponse(current_peer_address_, current_request_->As<BlockList>());
      }
      else
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Chain request to: ", ToBase64(current_peer_address_),
                       " failed. Reason: ", service::ToString(status));

        state_machine_->Delay(std::chrono::seconds{1});
        return State::REQUEST_HEAVIEST_CHAIN;
      }

      // clear the state
      current_peer_address_  = Address{};
      current_missing_block_ = BlockHash{};
      next_state             = State::SYNCHRONISED;
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Still waiting for heaviest chain response");
      state_machine_->Delay(std::chrono::seconds{1});
    }
  }

  return next_state;
}

MainChainRpcService::State MainChainRpcService::OnSynchronised(State current, State previous)
{
  state_synchronised_->increment();

  State next_state{State::SYNCHRONISED};

  FETCH_UNUSED(current);

  MainChain::BlockPtr head_of_chain = chain_.GetHeaviestBlock();
  uint64_t const      seconds_since_last_block =
      GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM)) -
      head_of_chain->timestamp;

  // Assume if the chain is quite old that there are peers who have a heavier chain we haven't
  // heard about
  if (seconds_since_last_block > 100 && head_of_chain->block_number != 0)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Synchronisation appears to be lost - chain is old.");
    state_machine_->Delay(std::chrono::milliseconds{1000});
    next_state = State::REQUEST_HEAVIEST_CHAIN;
  }
  else if (chain_.HasMissingBlocks())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Synchronisation lost - chain has missing blocks");

    next_state = State::SYNCHRONISING;
  }
  else if (previous != State::SYNCHRONISED)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Synchronised");
  }
  else
  {
    state_machine_->Delay(std::chrono::milliseconds{100});
  }

  return next_state;
}

}  // namespace ledger
}  // namespace fetch
