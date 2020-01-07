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
#include "ledger/protocols/main_chain_rpc_client_interface.hpp"
#include "logging/logging.hpp"
#include "muddle/packet.hpp"
#include "network/generics/milli_timer.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/utils/timer.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>

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

}  // namespace

MainChainRpcService::MainChainRpcService(MuddleEndpoint &             endpoint,
                                         MainChainRpcClientInterface &rpc_client, MainChain &chain,
                                         TrustSystem &trust, ConsensusPtr consensus)
  : muddle::rpc::Server(endpoint, SERVICE_MAIN_CHAIN, CHANNEL_RPC)
  , endpoint_(endpoint)
  , chain_(chain)
  , trust_(trust)
  , consensus_(std::move(consensus))
  , block_subscription_(endpoint.Subscribe(SERVICE_MAIN_CHAIN, CHANNEL_BLOCKS))
  , main_chain_protocol_(chain_)
  , rpc_client_(rpc_client)
  , state_machine_{std::make_shared<StateMachine>("MainChain", State::SYNCHRONISING,
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
  , recv_block_dirty_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_recv_block_dirty_total",
        " The total number of dirty blocks received from the network")}
  , state_synchronising_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_state_synchronising_total",
        "The number of times in the synchronisiing state")}
  , state_synchronised_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_state_synchronised_total",
        "The number of times in the sychronised state")}
  , state_start_sync_with_peer_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_state_rstart_sync_with_peer_total",
        "The number of times in the start sync with peer state")}
  , state_request_next_blocks_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_state_request_next_blocks_total",
        "The number of times in the request next blocks state")}
  , state_wait_for_next_blocks_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_state_wait_for_next_blocks_total",
        "The number of times in the wait for next blocks state")}
  , state_complete_sync_with_peer_{telemetry::Registry::Instance().CreateCounter(
        "ledger_mainchain_service_state_complete_sync_with_peer_total",
        "The number of times in the complete sync with peer state")}
  , state_current_{telemetry::Registry::Instance().CreateGauge<uint32_t>(
        "ledger_mainchain_service_state_complete_sync_with_peer_total",
        "The number of times in the complete sync with peer state")}
  , new_block_duration_{telemetry::Registry::Instance().CreateHistogram(
        {1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1, 1.0, 1e1, 1e2, 1e3},
        "ledger_mainchain_service_new_block_duration", "The duration of the new block handler")}
{
  assert(consensus_);

  // register the main chain protocol
  Add(RPC_MAIN_CHAIN, &main_chain_protocol_);

  // configure the state machine
  // clang-format off
  state_machine_->RegisterHandler(State::SYNCHRONISING,           this, &MainChainRpcService::OnSynchronising);
  state_machine_->RegisterHandler(State::SYNCHRONISED,            this, &MainChainRpcService::OnSynchronised);
  state_machine_->RegisterHandler(State::START_SYNC_WITH_PEER,  this, &MainChainRpcService::OnStartSyncWithPeer);
  state_machine_->RegisterHandler(State::REQUEST_NEXT_BLOCKS, this, &MainChainRpcService::OnRequestNextSetOfBlocks);
  state_machine_->RegisterHandler(State::WAIT_FOR_NEXT_BLOCKS,    this, &MainChainRpcService::OnWaitForBlocks);
  state_machine_->RegisterHandler(State::COMPLETE_SYNC_WITH_PEER,    this, &MainChainRpcService::OnCompleteSyncWithPeer);
  // clang-format on

  state_machine_->OnStateChange([](State current, State previous) {
    FETCH_UNUSED(current);
    FETCH_UNUSED(previous);
    FETCH_LOG_DEBUG(LOGGING_NAME, "Changed state: ", ToString(previous), " -> ", ToString(current));
  });

  // clear the restart timer
  resync_interval_.Restart(std::chrono::seconds{uint64_t{PERIODIC_RESYNC_SECONDS}});

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

    ++loose_blocks_seen_;
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
    ++loose_blocks_seen_;
    break;
  case BlockStatus::DUPLICATE:
    status_text = "Duplicate";
    recv_block_duplicate_count_->increment();
    break;
  case BlockStatus::INVALID:
    status_text = "Invalid";
    recv_block_invalid_count_->increment();
    break;
  case BlockStatus::DIRTY:
    status_text = "Dirty";
    recv_block_invalid_count_->increment();
    break;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "New Block: #", block.block_number, " 0x", block.hash.ToHex(),
                 " (from peer: ", ToBase64(from), " num txs: ", block.GetTransactionCount(),
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
  std::size_t dirty{0};

  for (auto it = begin; it != end; ++it)
  {
    // skip the genesis block
    if (it->IsGenesis())
    {
      continue;
    }

    // recompute the digest
    it->UpdateDigest();

    // add the block
    if (!ValidBlock(*it, "during fwd sync"))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Synced bad proof block: 0x", it->hash.ToHex(),
                      " from: muddle://", ToBase64(address));
      ++invalid;
      continue;
    }

    auto const status = chain_.AddBlock(*it);

    switch (status)
    {
    case BlockStatus::ADDED:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Synced new block: 0x", it->hash.ToHex(), " from: muddle://",
                      ToBase64(address));
      ++added;
      break;
    case BlockStatus::LOOSE:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Synced loose block: 0x", it->hash.ToHex(), " from: muddle://",
                      ToBase64(address));
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
    case BlockStatus::DIRTY:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Synced dirty block: 0x", it->hash.ToHex(), " from: muddle://",
                      ToBase64(address));
      ++dirty;
      break;
    }
  }

  if (invalid != 0u)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Synced Summary: Invalid: ", invalid, " Added: ", added,
                   " Loose: ", loose, " Duplicate: ", duplicate, " Dirty: ", dirty,
                   " from: muddle://", ToBase64(address));
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Synced Summary: Added: ", added, " Loose: ", loose,
                   " Duplicate: ", duplicate, " Dirty: ", dirty, " from: muddle://",
                   ToBase64(address));
  }
}

State MainChainRpcService::OnSynchronising()
{
  state_synchronising_->increment();
  state_current_->set(static_cast<uint32_t>(State::SYNCHRONISING));

  State next_state = State::SYNCHRONISED;

  // sync peer selection
  current_peer_address_ = GetRandomTrustedPeer();

  if (!current_peer_address_.empty())
  {
    next_state = State::START_SYNC_WITH_PEER;
  }

  return next_state;
}

State MainChainRpcService::OnSynchronised(State current, State previous)
{
  FETCH_UNUSED(current);

  state_synchronised_->increment();
  state_current_->set(static_cast<uint32_t>(State::SYNCHRONISED));

  State next_state{State::SYNCHRONISED};

  // Assume if the chain is quite old that there are peers who have a heavier chain we haven't
  // heard about
  if (loose_blocks_seen_ > 5)
  {
    loose_blocks_seen_ = 0;
    FETCH_LOG_INFO(LOGGING_NAME, "Synchronisation appears to be lost - loose blocks detected");

    next_state = State::SYNCHRONISING;
  }
  else if (previous != State::SYNCHRONISED)
  {
    // restart the interval timer
    resync_interval_.Restart(std::chrono::seconds{uint64_t{PERIODIC_RESYNC_SECONDS}});

    FETCH_LOG_INFO(LOGGING_NAME, "Synchronised");
  }
  else if (resync_interval_.HasExpired())
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Kicking forward sync periodically");

    next_state = State::SYNCHRONISING;
  }

  state_machine_->Delay(std::chrono::milliseconds{100});
  return next_state;
}

State MainChainRpcService::OnStartSyncWithPeer()
{
  state_start_sync_with_peer_->increment();
  state_current_->set(static_cast<uint32_t>(State::START_SYNC_WITH_PEER));

  // we always start a sync from the block 1 behind our heaviest block (except in the case of
  // genesis)
  block_resolving_ = chain_.GetHeaviestBlock();
  if (!block_resolving_->IsGenesis())
  {
    block_resolving_ = chain_.GetBlock(block_resolving_->previous_hash);
  }

  if (block_resolving_ && !current_peer_address_.empty())
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Resolving: #", block_resolving_->block_number, " 0x",
                    block_resolving_->hash.ToHex(), " from: muddle://",
                    current_peer_address_.ToBase64());
  }

  return State::REQUEST_NEXT_BLOCKS;
}

State MainChainRpcService::OnRequestNextSetOfBlocks()
{
  state_request_next_blocks_->increment();
  state_current_->set(static_cast<uint32_t>(State::REQUEST_NEXT_BLOCKS));

  // in some error cases we might be requested to request the next blocks of a null pointer. In this
  // case we simply conclude our transactions with this peer
  if (!(block_resolving_ && !current_peer_address_.empty()))
  {
    return State::COMPLETE_SYNC_WITH_PEER;
  }

  // make the time travel request
  current_request_ =
      rpc_client_.TimeTravel(current_peer_address_, block_resolving_->hash).GetInnerPromise();

  return State::WAIT_FOR_NEXT_BLOCKS;
}

State MainChainRpcService::OnWaitForBlocks()
{
  state_wait_for_next_blocks_->increment();
  state_current_->set(static_cast<uint32_t>(State::WAIT_FOR_NEXT_BLOCKS));

  // this should not happen as it would be a logical error, however, if it does simply restart the
  // sync process
  if (!current_request_)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "State machine error. Restarting sync");

    // something went wrong we should attempt to request the chain again
    state_machine_->Delay(std::chrono::milliseconds{500});
    return State::COMPLETE_SYNC_WITH_PEER;
  }

  // if we are still waiting for the promise to resolve
  auto const status = current_request_->state();
  if (status == PromiseState::WAITING)
  {
    state_machine_->Delay(std::chrono::milliseconds{100});
    return State::WAIT_FOR_NEXT_BLOCKS;
  }

  // at this point the promise has either resolved successfully or not.
  if ((status == PromiseState::FAILED) || (status == PromiseState::TIMEDOUT))
  {
    if (++consecutive_failures_ >= 3)
    {
      // too many failures give up on this peer
      block_resolving_ = {};
    }

    state_machine_->Delay(std::chrono::milliseconds{100 * consecutive_failures_});
    return State::REQUEST_NEXT_BLOCKS;
  }

  // If we have passed all these checks then we have successfully retrieved a travelogue from our
  // peer
  MainChainProtocol::Travelogue log{};
  if (!current_request_->GetResult(log))
  {
    // as soon as we get an invalid response from the peer we can simply conclude interacting with
    // them
    return State::COMPLETE_SYNC_WITH_PEER;
  }

  // Peer doesn't know of our reference block. This is normal in the case of forking. Simply walk
  // back down the chain until you find a block you can both agree on and then walk forward from
  // this point
  if (log.status == TravelogueStatus::NOT_FOUND)
  {
    // if the responding block was not found then walk back by one block
    block_resolving_ = chain_.GetBlock(block_resolving_->previous_hash);

    return State::REQUEST_NEXT_BLOCKS;
  }

  // We always expect at least 1 block to be synced during this process, failure to do this is not
  // normal and implies we should try and sync with another peer
  if (log.blocks.empty())
  {
    return State::COMPLETE_SYNC_WITH_PEER;
  }

  // process all of the blocks that have been returned from the syncing process
  HandleChainResponse(current_peer_address_, log.blocks.begin(), log.blocks.end());

  // we have now reached the heaviest tip
  auto const &latest_block = log.blocks.back();

  // check to see if we have either reached the heaviest tip or we are starting to advance past the
  // heaviest block number of the peer (presumably we are chasing an side branch)
  if ((latest_block.hash == log.heaviest_hash) || (latest_block.block_number > log.block_number))
  {
    block_resolving_ = {};
  }
  else
  {
    // we need to determine the next block on from the blocks that we are currently resolving
    // to request the information from
    for (auto it = log.blocks.rbegin(); it != log.blocks.rend(); ++it)
    {
      block_resolving_ = chain_.GetBlock(it->hash);
      if (block_resolving_)
      {
        break;
      }
    }
  }

  return State::REQUEST_NEXT_BLOCKS;
}

State MainChainRpcService::OnCompleteSyncWithPeer()
{
  state_complete_sync_with_peer_->increment();
  state_current_->set(static_cast<uint32_t>(State::COMPLETE_SYNC_WITH_PEER));

  current_peer_address_ = {};
  current_request_      = {};
  block_resolving_      = {};
  consecutive_failures_ = 0;

  return State::SYNCHRONISED;
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

}  // namespace ledger
}  // namespace fetch
