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
#include "ledger/chain/transaction_serialization.hpp"

namespace fetch {
namespace ledger {

const std::size_t TransactionStoreSyncService::BATCH_SIZE = 30;

TransactionStoreSyncService::TransactionStoreSyncService(
    uint32_t lane_id, MuddlePtr muddle, ObjectStorePtr store, LaneControllerPtr controller_ptr,
    std::size_t verification_threads, std::chrono::milliseconds the_timeout,
    std::chrono::milliseconds promise_wait_timeout,
    std::chrono::milliseconds fetch_object_wait_duration)
  : muddle_(std::move(muddle))
  , store_(std::move(store))
  , verifier_(*this, verification_threads, "TxV-L" + std::to_string(lane_id))
  , lane_controller_(std::move(controller_ptr))
  , time_duration_(the_timeout)
  , promise_wait_time_duration_(promise_wait_timeout)
  , fetch_object_wait_duration_(fetch_object_wait_duration)
  , id_(lane_id)
{
  client_ = std::make_shared<Client>("R:TxSync-L" + std::to_string(lane_id), muddle_->AsEndpoint(),
                                     Muddle::Address(), SERVICE_LANE, CHANNEL_RPC);

  this->Allow(State::QUERY_OBJECT_COUNTS, State::INITIAL)
      .Allow(State::RESOLVING_OBJECT_COUNTS, State::QUERY_OBJECT_COUNTS)
      .Allow(State::QUERY_SUBTREE, State::RESOLVING_OBJECT_COUNTS)
      .Allow(State::QUERY_SUBTREE, State::RESOLVING_SUBTREE)
      .Allow(State::RESOLVING_SUBTREE, State::QUERY_SUBTREE)
      .Allow(State::QUERY_OBJECTS, State::RESOLVING_SUBTREE)
      .Allow(State::RESOLVING_OBJECTS, State::QUERY_OBJECTS)
      .Allow(State::QUERY_OBJECTS, State::RESOLVING_OBJECTS)
      .Allow(State::TRIM_CACHE, State::RESOLVING_OBJECTS)
      .Allow(State::QUERY_OBJECTS, State::TRIM_CACHE);
}

TransactionStoreSyncService::~TransactionStoreSyncService()
{
  FETCH_LOG_WARN(LOGGING_NAME, "Lane ", id_, ": teardown sync service");
  muddle_->Shutdown();
  client_ = nullptr;
  muddle_ = nullptr;
  store_  = nullptr;
  FETCH_LOG_WARN(LOGGING_NAME, "Lane ", id_, ": sync service done!");
}

bool TransactionStoreSyncService::PossibleNewState(State &current_state)
{
  {
    FETCH_LOCK(mutex_);
    if (current_state == State::INITIAL && !timeout_set_)
    {
      SetTimeOut();
      return false;
    }
    if (current_state == State::INITIAL && timeout_.IsDue())
    {
      auto connections = lane_controller_->GetPeers();
      if (connections.size() == 0)
      {
        timeout_set_ = false;
        SetTimeOut();
        return false;
      }
      else
      {
        current_state = State::QUERY_OBJECT_COUNTS;
        fetch_object_wait_timeout_.Set(fetch_object_wait_duration_);
        return true;
      }
    }
  }
  bool result = false;
  switch (current_state)
  {
  case State::INITIAL:
  {
    result = false;
    break;
  }
  case State::QUERY_OBJECT_COUNTS:
  {
    auto active_connections = lane_controller_->GetPeers();
    for (auto const &connection : active_connections)
    {
      auto prom = PromiseOfObjectCount(client_->CallSpecificAddress(
          connection, RPC_TX_STORE_SYNC, TransactionStoreSyncProtocol::OBJECT_COUNT));
      pending_object_count_.Add(connection, prom);
    }
    FETCH_LOCK(mutex_);
    max_object_count_ = 0;
    current_state     = State::RESOLVING_OBJECT_COUNTS;
    promise_wait_timeout_.Set(time_duration_);
    result = true;
    break;
  }
  case State::RESOLVING_OBJECT_COUNTS:
  {
    auto counts = pending_object_count_.Resolve();
    FETCH_LOCK(mutex_);
    for (auto &result : pending_object_count_.Get(MAX_OBJECT_COUNT_RESOLUTION_PER_CYCLE))
    {
      max_object_count_ = std::max(max_object_count_, result.promised);
    }
    if (counts.failed > 0)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Lane ", id_, ": ", "Failed object count promises ",
                      counts.failed);
    }
    if (counts.pending > 0)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Lane ", id_, ": ", "Still waiting for object counts...");
      if (!promise_wait_timeout_.IsDue())
      {
        result = false;
        break;
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Lane ", id_, ": ",
                       "Still pending object count promises, but limit approached!");
      }
    }

    // If there are objects to sync from the network, fetch N roots from each of the peers in
    // parallel. So if we decided to split the sync into 4 roots, the mask would be 2 (bits) and
    // the roots to sync 00, 10, 01 and 11...
    // where roots to sync are all objects with the key starting with those bits
    if (max_object_count_ != 0)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Lane ", id_, ": ", "Expected tx size: ", max_object_count_);

      root_size_ = platform::Log2Ceil(((max_object_count_ / (PULL_LIMIT_ / 2)) + 1)) + 1;

      for (uint64_t i = 0, end = (1 << (root_size_)); i < end; ++i)
      {
        roots_to_sync_.push(Reverse(static_cast<uint8_t>(i)));
      }
    }
    current_state = State::QUERY_SUBTREE;
    result        = true;
    break;
  }
  case State::QUERY_SUBTREE:
  {
    auto active_connections = lane_controller_->GetPeers();
    FETCH_LOCK(mutex_);
    for (auto const &connection : active_connections)
    {
      if (roots_to_sync_.empty())
      {
        break;
      }

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
      result = false;
      break;
    }
    promise_wait_timeout_.Set(promise_wait_time_duration_);
    current_state = State::RESOLVING_SUBTREE;
    result        = true;
    break;
  }
  case State::RESOLVING_SUBTREE:
  {
    auto counts = pending_subtree_.Resolve();
    for (auto &result : pending_subtree_.Get(MAX_SUBTREE_RESOLUTION_PER_CYCLE))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Lane ", id_, ": ", "Got ", result.promised.size(),
                      " subtree objects!");

      for (auto &tx : result.promised)
      {
        verifier_.AddTransaction(tx.AsMutable());
      }
    }
    FETCH_LOCK(mutex_);
    if (counts.failed > 0)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Lane ", id_, ": ", "Failed subtree promises count ",
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
        if (roots_to_sync_.size() > 0)
        {
          current_state = State::QUERY_SUBTREE;
          result        = true;
          break;
        }
        result = false;
        break;
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Lane ", id_, ": ", "Timeout for subtree promises count!",
                       counts.pending);
        // get the pending
        auto pending = pending_subtree_.GetPending();
        for (auto &req : pending)
        {
          roots_to_sync_.push(promise_id_to_roots_[req.second.id()]);
        }
      }
    }
    promise_id_to_roots_.clear();
    current_state = roots_to_sync_.empty() ? State::QUERY_OBJECTS : State::QUERY_SUBTREE;
    result        = true;
    break;
  }
  case State::QUERY_OBJECTS:
  {
    if (!fetch_object_wait_timeout_.IsDue())
    {
      result = false;
      break;
    }
    auto active_connections = lane_controller_->GetPeers();
    for (auto const &connection : active_connections)
    {
      auto promise = PromiseOfTxList(client_->CallSpecificAddress(
          connection, RPC_TX_STORE_SYNC, TransactionStoreSyncProtocol::PULL_OBJECTS));
      pending_objects_.Add(connection, promise);
    }
    current_state = State::RESOLVING_OBJECTS;
    promise_wait_timeout_.Set(promise_wait_time_duration_);
    fetch_object_wait_timeout_.Set(fetch_object_wait_duration_);
    FETCH_LOCK(is_ready_mutex_);
    is_ready_ = true;
    result    = true;
    break;
  }
  case State::RESOLVING_OBJECTS:
  {
    auto counts = pending_objects_.Resolve();
    for (auto &result : pending_objects_.Get(MAX_OBJECT_RESOLUTION_PER_CYCLE))
    {
      if (!result.promised.empty())
      {
        FETCH_LOG_DEBUG(LOGGING_NAME, "Lane ", id_, ": ", "Got ", result.promised.size(),
                        " objects!");
      }

      for (auto &tx : result.promised)
      {
        verifier_.AddTransaction(tx.AsMutable());
      }
    }

    FETCH_LOCK(mutex_);
    if (counts.pending > 0)
    {
      if (!promise_wait_timeout_.IsDue())
      {
        result = false;
        break;
      }
      FETCH_LOG_WARN(LOGGING_NAME, "Lane ", id_, ": ",
                     "Still pending object promises but limit approached!");
    }

    if (counts.failed)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Lane ", id_, ": ", "Failed promises: ", counts.failed);
    }

    current_state = State::TRIM_CACHE;
    result        = true;
    break;
  }
  case State::TRIM_CACHE:
  {
    if (trim_cache_callback_)
    {
      trim_cache_callback_();
    }
    current_state = State::QUERY_OBJECTS;
    result        = true;
    break;
  }
  };
  return result;
}

void TransactionStoreSyncService::OnTransaction(VerifiedTransaction const &tx)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Verified Sync TX: ", tx.digest().ToBase64(), " (", tx.contract_name(), ')');

  ResourceID const rid(tx.digest());

  if (!store_->Has(rid))
  {
    store_->Set(rid, tx, true);
  }
}

}  // namespace ledger
}  // namespace fetch
