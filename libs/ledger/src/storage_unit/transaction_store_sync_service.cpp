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

#include "ledger/storage_unit/transaction_store_sync_service.hpp"
#include "core/macros.hpp"
#include "ledger/chain/transaction_serialization.hpp"

#include <cassert>
#include <chrono>
#include <memory>

static char const *FETCH_MAYBE_UNUSED ToString(fetch::ledger::tx_sync::State state)
{
  using State = fetch::ledger::tx_sync::State;

  char const *text = "Unknown";

  switch (state)
  {
  case State::INITIAL:
    text = "Initial";
    break;
  case State::QUERY_OBJECT_COUNTS:
    text = "Query Object Counts";
    break;
  case State::RESOLVING_OBJECT_COUNTS:
    text = "Resolving Object Counts";
    break;
  case State::QUERY_SUBTREE:
    text = "Query Subtree";
    break;
  case State::RESOLVING_SUBTREE:
    text = "Resolving Subtree";
    break;
  case State::QUERY_OBJECTS:
    text = "Query Objects";
    break;
  case State::RESOLVING_OBJECTS:
    text = "Resolving Objects";
    break;
  case State::TRIM_CACHE:
    text = "Trim Cache";
    break;
  }

  return text;
}

namespace fetch {
namespace ledger {

TransactionStoreSyncService::TransactionStoreSyncService(Config const &cfg, MuddlePtr muddle,
                                                         ObjectStorePtr store)
  : state_machine_{std::make_shared<core::StateMachine<State>>("TransactionStoreSyncService",
                                                               State::INITIAL)}
  , cfg_{cfg}
  , muddle_(std::move(muddle))
  , client_(std::make_shared<Client>("R:TxSync-L" + std::to_string(cfg_.lane_id),
                                     muddle_->AsEndpoint(), Muddle::Address(), SERVICE_LANE,
                                     CHANNEL_RPC))
  , store_(std::move(store))
  , verifier_(*this, cfg_.verification_threads, "TxV-L" + std::to_string(cfg_.lane_id))
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
    FETCH_LOG_DEBUG(LOGGING_NAME, "Updating state to: ", ToString(new_state));
  });
}

TransactionStoreSyncService::~TransactionStoreSyncService()
{
  FETCH_LOG_WARN(LOGGING_NAME, "Lane ", cfg_.lane_id, ": teardown sync service");
  muddle_->Shutdown();
  client_ = nullptr;
  muddle_ = nullptr;
  store_  = nullptr;
  FETCH_LOG_WARN(LOGGING_NAME, "Lane ", cfg_.lane_id, ": sync service done!");
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnInitial()
{
  if (muddle_->AsEndpoint().GetDirectlyConnectedPeers().empty())
  {
    return State::INITIAL;
  }

  return State::QUERY_OBJECT_COUNTS;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnQueryObjectCounts()
{
  for (auto const &connection : muddle_->AsEndpoint().GetDirectlyConnectedPeers())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Query objects from: muddle://", connection.ToBase64());

    auto prom = PromiseOfObjectCount(client_->CallSpecificAddress(
        connection, RPC_TX_STORE_SYNC, TransactionStoreSyncProtocol::OBJECT_COUNT));
    pending_object_count_.Add(connection, prom);
  }

  FETCH_LOCK(mutex_);
  max_object_count_ = 0;
  promise_wait_timeout_.Set(cfg_.main_timeout);

  return State::RESOLVING_OBJECT_COUNTS;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnResolvingObjectCounts()
{
  auto counts = pending_object_count_.Resolve();
  FETCH_LOCK(mutex_);
  for (auto &result : pending_object_count_.Get(MAX_OBJECT_COUNT_RESOLUTION_PER_CYCLE))
  {
    max_object_count_ = std::max(max_object_count_, result.promised);
  }
  if (counts.failed > 0)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ", "Failed object count promises ",
                    counts.failed);
  }
  if (counts.pending > 0)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ", "Still waiting for object counts...");
    if (!promise_wait_timeout_.IsDue())
    {
      state_machine_->Delay(std::chrono::milliseconds{20});

      return State::RESOLVING_OBJECT_COUNTS;
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ",
                     "Still pending object count promises, but limit approached!");
    }
  }

  // If there are objects to sync from the network, fetch N roots from each of the peers in
  // parallel. So if we decided to split the sync into 4 roots, the mask would be 2 (bits) and
  // the roots to sync 00, 10, 01 and 11...
  // where roots to sync are all objects with the key starting with those bits
  if (max_object_count_ == 0)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Network appears to have no transactions! Number of peers: ",
                   muddle_->AsEndpoint().GetDirectlyConnectedPeers().size());
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ",
                   "Expected tx size: ", max_object_count_);

    root_size_ = platform::Log2Ceil(((max_object_count_ / (PULL_LIMIT_ / 2)) + 1)) + 1;

    for (uint64_t i = 0, end = (1u << root_size_); i < end; ++i)
    {
      roots_to_sync_.push(Reverse(static_cast<uint8_t>(i)));
    }
  }

  if (roots_to_sync_.empty())
  {
    state_machine_->Delay(std::chrono::milliseconds{20});

    return State::QUERY_OBJECT_COUNTS;
  }

  return State::QUERY_SUBTREE;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnQuerySubtree()
{
  assert(!roots_to_sync_.empty());

  FETCH_LOCK(mutex_);
  for (auto const &connection : muddle_->AsEndpoint().GetDirectlyConnectedPeers())
  {
    auto root = roots_to_sync_.front();
    roots_to_sync_.pop();

    byte_array::ByteArray transactions_prefix;

    transactions_prefix.Resize(std::size_t{ResourceID::RESOURCE_ID_SIZE_IN_BYTES});
    transactions_prefix[0] = root;

    auto promise = PromiseOfTxList(client_->CallSpecificAddress(
        connection, RPC_TX_STORE_SYNC, TransactionStoreSyncProtocol::PULL_SUBTREE,
        transactions_prefix, root_size_));

    promise_id_to_roots_[promise.id()] = root;
    pending_subtree_.Add(root, promise);
  }

  if (!roots_to_sync_.empty())
  {
    return State::QUERY_SUBTREE;
  }

  promise_wait_timeout_.Set(cfg_.promise_wait_timeout);

  return State::RESOLVING_SUBTREE;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnResolvingSubtree()
{
  auto counts = pending_subtree_.Resolve();

  std::size_t synced_tx{0};
  for (auto &result : pending_subtree_.Get(MAX_SUBTREE_RESOLUTION_PER_CYCLE))
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ", "Got ", result.promised.size(),
                    " subtree objects!");

    for (auto &tx : result.promised)
    {
      // add the transaction to the verifier
      verifier_.AddTransaction(tx.AsMutable());

      ++synced_tx;
    }
  }

  if (synced_tx)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Lane ", cfg_.lane_id, " Incorporated ", synced_tx, " txs");
  }

  FETCH_LOCK(mutex_);
  if (counts.failed > 0)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ", "Failed subtree promises count ",
                   counts.failed);
    for (auto &fail : pending_subtree_.GetFailures(MAX_SUBTREE_RESOLUTION_PER_CYCLE))
    {
      roots_to_sync_.push(promise_id_to_roots_[fail.promise.id()]);
    }
  }
  if (counts.pending > 0)
  {
    if (!promise_wait_timeout_.IsDue())
    {
      if (!roots_to_sync_.empty())
      {
        return State::QUERY_SUBTREE;
      }

      return State::RESOLVING_SUBTREE;
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ",
                     "Timeout for subtree promises count!", counts.pending);
      // get the pending
      auto pending = pending_subtree_.GetPending();
      for (auto &req : pending)
      {
        roots_to_sync_.push(promise_id_to_roots_[req.second.id()]);
      }
    }
  }

  promise_id_to_roots_.clear();

  return roots_to_sync_.empty() ? State::QUERY_OBJECTS : State::QUERY_SUBTREE;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnQueryObjects()
{
  if (!fetch_object_wait_timeout_.IsDue())
  {
    return State::QUERY_OBJECTS;
  }

  for (auto const &connection : muddle_->AsEndpoint().GetDirectlyConnectedPeers())
  {
    auto promise = PromiseOfTxList(client_->CallSpecificAddress(
        connection, RPC_TX_STORE_SYNC, TransactionStoreSyncProtocol::PULL_OBJECTS));
    pending_objects_.Add(connection, promise);
  }

  promise_wait_timeout_.Set(cfg_.promise_wait_timeout);
  fetch_object_wait_timeout_.Set(cfg_.fetch_object_wait_duration);

  FETCH_LOCK(is_ready_mutex_);
  is_ready_ = true;

  return State::RESOLVING_OBJECTS;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnResolvingObjects()
{
  auto counts = pending_objects_.Resolve();

  std::size_t synced_tx{0};
  for (auto &result : pending_objects_.Get(MAX_OBJECT_RESOLUTION_PER_CYCLE))
  {
    if (!result.promised.empty())
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ", "Got ", result.promised.size(),
                      " objects!");
    }

    for (auto &tx : result.promised)
    {
      verifier_.AddTransaction(tx.AsMutable());
      ++synced_tx;
    }
  }

  if (synced_tx)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Lane ", cfg_.lane_id, " Pulled ", synced_tx, " txs");
  }

  FETCH_LOCK(mutex_);
  if (counts.pending > 0)
  {
    if (!promise_wait_timeout_.IsDue())
    {
      return State::RESOLVING_OBJECTS;
    }
    FETCH_LOG_WARN(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ",
                   "Still pending object promises but limit approached!");
  }

  if (counts.failed)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Lane ", cfg_.lane_id, ": ", "Failed promises: ", counts.failed);
  }

  return State::TRIM_CACHE;
}

TransactionStoreSyncService::State TransactionStoreSyncService::OnTrimCache()
{
  if (trim_cache_callback_)
  {
    trim_cache_callback_();
  }

  return State::QUERY_OBJECTS;
}

void TransactionStoreSyncService::OnTransaction(VerifiedTransaction const &tx)
{
  ResourceID const rid(tx.digest());

  if (!store_->Has(rid))
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Verified Sync TX: ", tx.digest().ToBase64(), " (",
                    tx.contract_name(), ')');

    store_->Set(rid, tx, true);
  }
}

}  // namespace ledger
}  // namespace fetch
