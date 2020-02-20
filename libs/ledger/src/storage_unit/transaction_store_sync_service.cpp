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

#include "chain/transaction_rpc_serializers.hpp"
#include "core/macros.hpp"
#include "ledger/storage_unit/transaction_storage_engine_interface.hpp"
#include "ledger/storage_unit/transaction_store_sync_service.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/registry.hpp"

#include <cassert>
#include <chrono>
#include <memory>

using namespace std::chrono_literals;

static constexpr std::size_t MAX_REQUESTS_PER_NODE = 2;

static char const *FETCH_MAYBE_UNUSED ToString(fetch::ledger::tx_sync::State state)
{
  using State = fetch::ledger::tx_sync::State;

  switch (state)
  {
  case State::INITIAL:
    return "Initial";
  case State::QUERY_OBJECT_COUNTS:
    return "Query Object Counts";
  case State::RESOLVING_OBJECT_COUNTS:
    return "Resolving Object Counts";
  case State::QUERY_SUBTREE:
    return "Query Subtree";
  case State::RESOLVING_SUBTREE:
    return "Resolving Subtree";
  case State::QUERY_OBJECTS:
    return "Query Objects";
  case State::RESOLVING_OBJECTS:
    return "Resolving Objects";
  case State::TRIM_CACHE:
    return "Trim Cache";
  }

  return "Unknown";
}

namespace fetch {
namespace ledger {

TransactionStoreSyncService::TransactionStoreSyncService(Config const &cfg, MuddleEndpoint &muddle,
                                                         TransactionStorageEngineInterface &store,
                                                         TxFinderProtocol *tx_finder_protocol,
                                                         TrimCacheCallback trim_cache_callback)
  : trim_cache_callback_(std::move(trim_cache_callback))
  , state_machine_{std::make_shared<core::StateMachine<State>>("TransactionStoreSyncService",
                                                               State::INITIAL)}
  , tx_finder_protocol_(tx_finder_protocol)
  , cfg_{cfg}
  , muddle_(muddle)
  , client_(std::make_shared<Client>("R:TxSync-L" + std::to_string(cfg_.lane_id), muddle,
                                     SERVICE_LANE, CHANNEL_RPC))
  , store_(store)
  , verifier_(*this, cfg_.verification_threads, "TxV-L" + std::to_string(cfg_.lane_id))
  , stored_transactions_{telemetry::Registry::Instance().CreateCounter(
        "ledger_tx_store_sync_service_stored_transactions_total",
        "Total number of all transactions received & stored by TransactionStoreSyncService")}
  , resolve_count_failures_{telemetry::Registry::Instance().CreateCounter(
        "ledger_tx_store_sync_service_resolve_count_failures_total",
        "Total number of failures to query the object count from a remote host")}
  , subtree_requests_total_{telemetry::Registry::Instance().CreateCounter(
        "ledger_tx_store_sync_service_subtree_request_total",
        "Total subtree requests made by the service")}
  , subtree_response_total_{telemetry::Registry::Instance().CreateCounter(
        "ledger_tx_store_sync_service_subtree_response_total",
        "Total number of subtree successful responses from a remote host")}
  , subtree_failure_total_{telemetry::Registry::Instance().CreateCounter(
        "ledger_tx_store_sync_service_subtree_failure_total",
        "The total number of subtree request failures observed")}
  , current_tss_state_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "current_tss_state", "The state in the state machine of the tx store")}
  , current_tss_peers_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "current_tss_peers", "The number of peers the sync can use")}
{
  state_machine_->RegisterHandler(State::INITIAL, this, &TransactionStoreSyncService::OnInitial);
  state_machine_->RegisterHandler(State::QUERY_OBJECT_COUNTS, this,
                                  &TransactionStoreSyncService::OnQueryObjectCounts);
  state_machine_->RegisterHandler(State::RESOLVING_OBJECT_COUNTS, this,
                                  &TransactionStoreSyncService::OnResolvingObjectCounts);
  state_machine_->RegisterHandler(State::QUERY_SUBTREE, this,
                                  &TransactionStoreSyncService::OnQuerySubtree);
  state_machine_->RegisterHandler(State::RESOLVING_SUBTREE, this,
                                  &TransactionStoreSyncService::OnResolvingSubtree);
  state_machine_->RegisterHandler(State::QUERY_OBJECTS, this,
                                  &TransactionStoreSyncService::OnQueryObjects);
  state_machine_->RegisterHandler(State::RESOLVING_OBJECTS, this,
                                  &TransactionStoreSyncService::OnResolvingObjects);
  state_machine_->RegisterHandler(State::TRIM_CACHE, this,
                                  &TransactionStoreSyncService::OnTrimCache);

  state_machine_->OnStateChange([](State new_state, State /* old_state */) {
    FETCH_UNUSED(new_state);
    FETCH_LOG_DEBUG(LOGGING_NAME, "*** Updating state to: ", ToString(new_state));
  });
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnInitial()
{
  current_tss_state_->set(static_cast<uint64_t>(state_machine_->state()));
  current_tss_peers_->set(muddle_.GetDirectlyConnectedPeers().size());

  if (muddle_.GetDirectlyConnectedPeers().empty())
  {
    return State::INITIAL;
  }

  return State::QUERY_OBJECT_COUNTS;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnQueryObjectCounts()
{
  current_tss_state_->set(static_cast<uint64_t>(state_machine_->state()));
  current_tss_peers_->set(muddle_.GetDirectlyConnectedPeers().size());

  for (auto const &connection : muddle_.GetDirectlyConnectedPeers())
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Query objects from: muddle://", connection.ToBase64());

    auto prom = PromiseOfObjectCount(client_->CallSpecificAddress(
        connection, RPC_TX_STORE_SYNC, TransactionStoreSyncProtocol::OBJECT_COUNT));
    pending_object_count_.Add(connection, prom);
  }

  max_object_count_ = 0;
  promise_wait_timeout_.Set(cfg_.main_timeout);

  return State::RESOLVING_OBJECT_COUNTS;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnResolvingObjectCounts()
{
  current_tss_state_->set(static_cast<uint64_t>(state_machine_->state()));
  auto const counts    = pending_object_count_.Resolve();
  auto const completed = pending_object_count_.Get(MAX_OBJECT_COUNT_RESOLUTION_PER_CYCLE);
  pending_object_count_.DiscardFailures();

  for (auto &result : completed)
  {
    max_object_count_ = std::max(max_object_count_, result.promised);
  }

  if (counts.failed > 0)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ",
                    "Failed object count promises: ", counts.failed);

    resolve_count_failures_->add(static_cast<uint64_t>(counts.failed));
  }

  if (counts.pending > 0)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ", "Still waiting for ", counts.pending,
                    " object count promises...");

    if (!promise_wait_timeout_.IsDue())
    {
      state_machine_->Delay(std::chrono::milliseconds{20});

      return State::RESOLVING_OBJECT_COUNTS;
    }

    FETCH_LOG_WARN(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ", "Still pending ", counts.pending,
                   " object count promises, but have reached timeout!");
  }

  // If there are objects to sync from the network, fetch N roots from each of the peers in
  // parallel. So if we decided to split the sync into 4 roots, the mask would be 2 (bits) and
  // the roots to sync 00, 10, 01 and 11...
  // where roots to sync are all objects with the key starting with those bits
  if (max_object_count_ == 0)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Network appears to have no transactions! Number of peers: ",
                   muddle_.GetDirectlyConnectedPeers().size());
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ",
                   "Expected tx count to sync: ", max_object_count_);

    root_size_ = platform::Log2Ceil((max_object_count_ / PULL_LIMIT) + 1) + 1;
    uint64_t const end{1ull << root_size_};
    for (uint64_t i = 0; i < end; ++i)
    {
      roots_to_sync_.emplace(i);
    }
  }

  if (roots_to_sync_.empty())
  {
    state_machine_->Delay(std::chrono::milliseconds{2000});

    return State::QUERY_OBJECT_COUNTS;
  }

  return State::QUERY_SUBTREE;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnQuerySubtree()
{
  current_tss_state_->set(static_cast<uint64_t>(state_machine_->state()));
  assert(!roots_to_sync_.empty());
  auto const orig_num_of_roots{roots_to_sync_.size()};

  auto const directly_connected_peers = muddle_.GetDirectlyConnectedPeers();

  std::size_t const maximum_inflight = MAX_REQUESTS_PER_NODE * directly_connected_peers.size();
  std::size_t const total_inflight   = pending_subtree_.GetNumPending();
  std::size_t const roots_to_query = maximum_inflight - std::min(total_inflight, maximum_inflight);

  // sanity check that this is not the case
  std::size_t requests_made{0};
  for (auto const &connection : directly_connected_peers)
  {
    // if there are no further roots to sync then we need to exit
    if (roots_to_sync_.empty())
    {
      break;
    }

    // if we have reached the maximum inflight requests
    if (requests_made >= roots_to_query)
    {
      break;
    }

    // extract the next root to sync
    auto root = roots_to_sync_.front();
    roots_to_sync_.pop();

    byte_array::ByteArray transactions_prefix;
    transactions_prefix.Resize(std::size_t{ResourceID::RESOURCE_ID_SIZE_IN_BYTES});
    *reinterpret_cast<decltype(root) *>(transactions_prefix.char_pointer()) = root;

    auto promise = PromiseOfTxList(client_->CallSpecificAddress(
        connection, RPC_TX_STORE_SYNC, TransactionStoreSyncProtocol::PULL_SUBTREE,
        transactions_prefix, root_size_));

    promise_id_to_roots_[promise.id()] = root;
    pending_subtree_.Add(root, promise);

    subtree_requests_total_->increment();
    ++requests_made;
  }

  promise_wait_timeout_.Set(cfg_.promise_wait_timeout);

  FETCH_LOG_INFO(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ", "QueryingSubtree: requested ",
                 orig_num_of_roots - roots_to_sync_.size(),
                 " root(s). Remaining roots to sync: ", roots_to_sync_.size(), " / ",
                 uint64_t{1ull << root_size_});

  return State::RESOLVING_SUBTREE;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnResolvingSubtree()
{
  current_tss_state_->set(static_cast<uint64_t>(state_machine_->state()));
  auto counts = pending_subtree_.Resolve();

  // resolve the sub-trees promises
  std::size_t synced_tx{0};
  for (auto &result : pending_subtree_.Get(MAX_SUBTREE_RESOLUTION_PER_CYCLE))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ", "Got ", result.promised.size(),
                   " subtree objects!");

    for (auto &tx : result.promised)
    {
      // this transaction is not recent
      tx.SetFromSubtreeSync();
      // add the transaction to the verifier
      verifier_.AddTransaction(std::make_shared<chain::Transaction>(tx));

      ++synced_tx;
    }

    subtree_response_total_->increment();
  }

  // report the number of incorporated transactions
  if (synced_tx != 0u)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Lane ", cfg_.lane_id, " Incorporated ", synced_tx, " TXs");
  }

  if (counts.failed > 0)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ", "Failed subtree promises count ",
                   counts.failed);

    for (auto &fail : pending_subtree_.GetFailures(MAX_SUBTREE_RESOLUTION_PER_CYCLE))
    {
      roots_to_sync_.push(promise_id_to_roots_[fail.promise.id()]);
    }

    subtree_failure_total_->add(static_cast<uint64_t>(counts.failed));
  }

  // evaluate if the syncing process if complete, this can only be the case when there are no in
  // flight requests and we have successfully evaluated all the roots we are after
  bool const is_subtree_sync_complete = roots_to_sync_.empty() && counts.pending == 0;
  if (!is_subtree_sync_complete)
  {
    state_machine_->Delay(10ms);
    return State::QUERY_SUBTREE;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Completed sub-tree syncing");

  // cleanup
  promise_id_to_roots_.clear();

  // if we get this far then we have completed the subtree sync process
  return State::QUERY_OBJECTS;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnQueryObjects()
{
  current_tss_state_->set(static_cast<uint64_t>(state_machine_->state()));
  DigestSet digests{};
  digests.reserve(TX_FINDER_PROTO_LIMIT);

  // collect up all the explicitly requested transactions from the block coordinator process
  Digest digest;
  while (digests.size() < TX_FINDER_PROTO_LIMIT && tx_finder_protocol_->Pop(digest))
  {
    digests.emplace(digest);
  }

  // Early exit: If it is not time to request the recent transaction and there are no explicit
  // requests for transactions then we should simply hold in this state

  bool const need_to_request_specific = !digests.empty();

  // Note: ONLY make one rpc call here to a client since there is a bug
  // when doing multiple adds to the pending_objects_ with the same connection
  bool const is_time_to_pull = fetch_object_wait_timeout_.IsDue() && !need_to_request_specific;

  if (!need_to_request_specific && !is_time_to_pull)
  {
    state_machine_->Delay(10ms);
    return State::QUERY_OBJECTS;
  }

  // walk through all
  for (auto const &connection : muddle_.GetDirectlyConnectedPeers())
  {
    // if it is time to pull the recent transactions then pull them
    if (is_time_to_pull)
    {
      auto p1 = PromiseOfTxList(client_->CallSpecificAddress(
          connection, RPC_TX_STORE_SYNC, TransactionStoreSyncProtocol::PULL_OBJECTS));
      if (!pending_objects_.Add(connection, p1))
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to add promise of transactions to queue");
      }

      FETCH_LOG_DEBUG(LOGGING_NAME, "Lane ", cfg_.lane_id, ": Periodically requesting recent TXs");
    }

    if (need_to_request_specific)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Lane ", cfg_.lane_id, ": Explicitly requesting ",
                     digests.size(), " TXs");

      auto p2 = PromiseOfTxList(client_->CallSpecificAddress(
          connection, RPC_TX_STORE_SYNC, TransactionStoreSyncProtocol::PULL_SPECIFIC_OBJECTS,
          digests));
      if (!pending_objects_.Add(connection, p2))
      {
        FETCH_LOG_WARN(LOGGING_NAME,
                       "Failed to add promise of transactions to queue - call specific");
      }
    }
  }

  promise_wait_timeout_.Set(cfg_.promise_wait_timeout);
  if (is_time_to_pull)
  {
    fetch_object_wait_timeout_.Set(cfg_.fetch_object_wait_duration);
  }

  is_ready_ = true;

  return State::RESOLVING_OBJECTS;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnResolvingObjects()
{
  current_tss_state_->set(static_cast<uint64_t>(state_machine_->state()));
  auto counts = pending_objects_.Resolve();

  std::size_t synced_tx{0};
  for (auto &result : pending_objects_.Get(MAX_OBJECT_RESOLUTION_PER_CYCLE))
  {
    if (!result.promised.empty())
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Lane ", cfg_.lane_id, ": Got ", result.promised.size(),
                      " objects!");
    }

    for (auto &tx : result.promised)
    {
      verifier_.AddTransaction(std::make_shared<chain::Transaction>(tx));
      ++synced_tx;
    }
  }

  if (synced_tx != 0u)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Lane ", cfg_.lane_id, " Synchronised ", synced_tx,
                    " requested txs");
  }

  if (counts.pending > 0)
  {
    if (!promise_wait_timeout_.IsDue())
    {
      state_machine_->Delay(10ms);
      return State::RESOLVING_OBJECTS;
    }
    FETCH_LOG_WARN(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ",
                   "Still pending object promises but timeout approached!");
  }

  if (counts.failed != 0u)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ", "Failed promises: ", counts.failed);
  }

  return State::TRIM_CACHE;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnTrimCache()
{
  current_tss_state_->set(static_cast<uint64_t>(state_machine_->state()));
  if (trim_cache_callback_)
  {
    trim_cache_callback_();
  }

  return State::QUERY_OBJECTS;
}

void TransactionStoreSyncService::OnTransaction(TransactionPtr const &tx)
{
  ResourceID const rid(tx->digest());

  if (!store_.Has(tx->digest()))
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Verified Sync TX: ", tx->digest().ToBase64(), " (",
                    tx->contract_address().display(), ')');

    // This transaction is recent unless it was received at subtree sync.
    store_.Add(*tx, !tx->IsFromSubtreeSync());
    stored_transactions_->increment();
  }
}

}  // namespace ledger
}  // namespace fetch
