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

#include "chain/transaction_layout_rpc_serializers.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "crypto/fetch_identity.hpp"
#include "ledger/chain/block_coordinator.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/consensus/consensus_interface.hpp"
#include "logging/logging.hpp"
#include "muddle/packet.hpp"
#include "network/generics/milli_timer.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/utils/timer.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>

static const uint64_t MAX_SUB_CHAIN_SIZE = 1000;

namespace fetch {
namespace ledger {
namespace {

using fetch::byte_array::ToBase64;
using fetch::muddle::Packet;

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
  default:;
  }

  return State::REQUEST_HEAVIEST_CHAIN;
}

}  // namespace

MainChainRpcService::MainChainRpcService(MuddleEndpoint &endpoint, MainChain &chain,
                                         TrustSystem &trust, Mode mode, ConsensusPtr consensus)
  : muddle::rpc::Server(endpoint, SERVICE_MAIN_CHAIN, CHANNEL_RPC)
  , mode_(mode)
  , endpoint_(endpoint)
  , chain_(chain)
  , trust_(trust)
  , consensus_(std::move(consensus))
  , block_subscription_(endpoint.Subscribe(SERVICE_MAIN_CHAIN, CHANNEL_BLOCKS))
  , main_chain_protocol_(chain_)
  , rpc_client_("R:MChain", endpoint, SERVICE_MAIN_CHAIN, CHANNEL_RPC)
  , state_machine_{std::make_shared<StateMachine>("MainChain", GetInitialState(mode),
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
  , new_block_duration_{telemetry::Registry::Instance().CreateHistogram(
        {1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1, 1.0, 1e1, 1e2, 1e3},
        "ledger_mainchain_service_new_block_duration", "The duration of the new block handler")}
{
  assert(consensus_);

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
    FETCH_UNUSED(current);
    FETCH_UNUSED(previous);
    FETCH_LOG_DEBUG(LOGGING_NAME, "Changed state: ", ToString(previous), " -> ", ToString(current));
  });

  // set the main chain rpc sync to accept gossip blocks
  block_subscription_->SetMessageHandler([this](Address const &from, uint16_t, uint16_t, uint16_t,
                                                Packet::Payload const &payload,
                                                Address                transmitter) {
    telemetry::FunctionTimer timer{*new_block_duration_};

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

  trust_.AddFeedback(transmitter, p2p::TrustSubject::BLOCK, p2p::TrustQuality::NEW_INFORMATION);

  if (!ValidBlock(block, "new block"))
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Gossiped block did not prove valid. Loose blocks seen: ", loose_blocks_seen_);
    loose_blocks_seen_++;
    return;
  }

  // add the new block to the chain
  auto const status = chain_.AddBlock(block);

  char const *status_text = "Unknown";
  switch (status)
  {
  case BlockStatus::ADDED:
    status_text = "Added";
    recv_block_valid_count_->increment();
    break;
  case BlockStatus::LOOSE:
    status_text = "Loose";
    recv_block_loose_count_->increment();
    break;
  case BlockStatus::DUPLICATE:
    status_text = "Duplicate";
    recv_block_duplicate_count_->increment();
    break;
  case BlockStatus::INVALID:
    status_text = "Invalid";
    recv_block_invalid_count_->increment();
    break;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "New Block: 0x", block.hash.ToHex(), " (from peer: ", ToBase64(from),
                 " num txs: ", block.GetTransactionCount(), " num: ", block.block_number,
                 " status: ", status_text, ")");
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

bool MainChainRpcService::ValidBlock(Block const &block, char const *action) const
{
  try
  {
    return !consensus_ || consensus_->ValidBlock(block) == ConsensusInterface::Status::YES;
  }
  catch (std::runtime_error const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Exception in consensus on validating ", action, ": ", ex.what());
    return false;
  }
}

void MainChainRpcService::HandleChainResponse(Address const &address, BlockList blocks)
{
  // default expectations is that blocks are returned in reverse order, later-to-earlier
  HandleChainResponse(address, blocks.rbegin(), blocks.rend());
}

template <class Begin, class End>
void MainChainRpcService::HandleChainResponse(Address const &address, Begin begin, End end)
{
  std::size_t added{0};
  std::size_t loose{0};
  std::size_t duplicate{0};
  std::size_t invalid{0};

  for (auto it = begin; it != end; ++it)
  {
    // skip the genesis block
    if ((*it)->IsGenesis())
    {
      continue;
    }

    auto block = std::const_pointer_cast<Block>(*it);

    // recompute the digest
    block->UpdateDigest();

    // add the block
    if (!ValidBlock(*block, "during fwd sync"))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Synced bad proof block: 0x", block->hash.ToHex(),
                      " from: muddle://", ToBase64(address));
      ++invalid;
      continue;
    }

    auto const status = chain_.AddBlock(block);

    switch (status)
    {
    case BlockStatus::ADDED:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Synced new block: 0x", block->hash.ToHex(), " from: muddle://",
                      ToBase64(address));
      ++added;
      break;
    case BlockStatus::LOOSE:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Synced loose block: 0x", block->hash.ToHex(),
                      " from: muddle://", ToBase64(address));
      ++loose;
      break;
    case BlockStatus::DUPLICATE:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Synced duplicate block: 0x", block->hash.ToHex(),
                      " from: muddle://", ToBase64(address));
      ++duplicate;
      break;
    case BlockStatus::INVALID:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Synced invalid block: 0x", block->hash.ToHex(),
                      " from: muddle://", ToBase64(address));
      ++invalid;
      break;
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

    if (!block_resolving_)
    {
      block_resolving_ = chain_.GetHeaviestBlock();
    }

    if (!block_resolving_)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to get heaviest block from the main chain!");
      state_machine_->Delay(std::chrono::milliseconds{1000});
      return next_state;
    }

    current_request_ =
        rpc_client_.CallSpecificAddress(current_peer_address_, RPC_MAIN_CHAIN,
                                        MainChainProtocol::TIME_TRAVEL, block_resolving_->hash);

    FETCH_LOG_INFO(LOGGING_NAME, "Attempting to resolve: ", block_resolving_->block_number,
                   " aka 0x", block_resolving_->hash.ToHex(), " Note: gen is: 0x",
                   chain::GetGenesisDigest().ToHex(), " from: muddle://",
                   current_peer_address_.ToBase64());

    next_state = State::WAIT_FOR_HEAVIEST_CHAIN;
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "No peers available to query from");
    return State::SYNCHRONISED;
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
    FETCH_LOG_WARN(LOGGING_NAME, "Issue found when requesting heaviest chain. Re-attempt.");
    // something went wrong we should attempt to request the chain again
    next_state = State::REQUEST_HEAVIEST_CHAIN;
    state_machine_->Delay(std::chrono::milliseconds{500});
  }
  else
  {
    // determine the status of the request that is in flight
    auto const status = current_request_->state();

    if (status != PromiseState::WAITING)
    {
      if (status == PromiseState::SUCCESS)
      {
        // the request was successful
        next_state = State::REQUEST_HEAVIEST_CHAIN;  // request succeeding chunk

        MainChainProtocol::Travelogue response{};
        if (current_request_->GetResult(response))
        {
          auto &blocks = response.blocks;

          // we should receive at least one extra block in addition to what we already have
          if (!blocks.empty())
          {
            block_resolving_ = {};

            HandleChainResponse(current_peer_address_, blocks.begin(), blocks.end());
            auto const &latest_hash = blocks.back()->hash;
            assert(!latest_hash.empty());  // should be set by HandleChainResponse()

            // TODO(unknown): this is to be improved later
            if (latest_hash == response.heaviest_hash)
            {
              return State::SYNCHRONISED;  // we have reached the tip
            }
          }

          uint64_t const attempting_to_resolve =
              !block_resolving_ ? 0 : block_resolving_->block_number;

          if (!block_resolving_)
          {
            FETCH_LOG_WARN(LOGGING_NAME, "No block set (?)");
          }

          if (blocks.empty() || response.not_on_heaviest ||
              (response.block_number > (attempting_to_resolve + 10)))
          {
            if (block_resolving_)
            {
              block_resolving_ = chain_.GetBlock(block_resolving_->previous_hash);
            }

            FETCH_LOG_WARN(
                LOGGING_NAME,
                "Received indication we are on a dead fork. Walking back. Peer heaviest: ",
                response.block_number, " blocks: ", blocks.size(),
                " heaviest: ", response.heaviest_hash.ToBase64());

            if (block_resolving_)
            {
              FETCH_LOG_WARN(LOGGING_NAME, "Walked to: ", block_resolving_->block_number);
            }
          }
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME,
                         "Received empty block response when forward syncing the chain!");
          state_machine_->Delay(std::chrono::seconds{8});
          next_state = GetInitialState(mode_);
        }
      }
      else
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Heaviest chain request to: ", ToBase64(current_peer_address_),
                       " failed. Reason: ", service::ToString(status));

        state_machine_->Delay(std::chrono::seconds{1});
        next_state = GetInitialState(mode_);
      }

      // clear the state
      current_peer_address_ = Address{};
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
                   " for block 0x", current_missing_block_.ToHex());

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
        BlockList blocks{};

        // the request was successful, simply hand off the blocks to be added to the chain
        if (current_request_->GetResult(blocks))
        {
          HandleChainResponse(current_peer_address_, blocks);
        }
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

MainChainRpcService::State MainChainRpcService::OnSynchronised()
{
  State next_state{State::SYNCHRONISED};
  state_synchronised_->increment();

  // reset the timer if we have just transitioned from another state
  if (state_machine_->previous_state() != State::SYNCHRONISED)
  {
    timer_to_proceed_.Restart(std::chrono::seconds{uint64_t{PERIODIC_RESYNC_SECONDS}});
  }

  MainChain::BlockPtr head_of_chain = chain_.GetHeaviestBlock();

  // Assume if the chain is quite old that there are peers who have a heavier chain we haven't
  // heard about
  if (loose_blocks_seen_ > 5)
  {
    loose_blocks_seen_ = 0;
    FETCH_LOG_INFO(LOGGING_NAME, "Synchronisation appears to be lost - loose blocks detected");
    state_machine_->Delay(std::chrono::milliseconds{1000});
    next_state = State::REQUEST_HEAVIEST_CHAIN;
  }
  else if (timer_to_proceed_.HasExpired())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Kicking forward sync periodically");
    next_state = State::REQUEST_HEAVIEST_CHAIN;
  }
  else if (state_machine_->previous_state() != State::SYNCHRONISED)
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
