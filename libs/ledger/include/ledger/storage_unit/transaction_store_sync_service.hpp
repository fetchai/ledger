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

#include "core/service_ids.hpp"
#include "ledger/storage_unit/lane_controller.hpp"
#include "ledger/storage_unit/transaction_sinks.hpp"
#include "ledger/transaction_verifier.hpp"
#include "network/generics/atomic_state_machine.hpp"
#include "network/generics/future_timepoint.hpp"
#include "network/generics/promise_of.hpp"
#include "network/generics/requesting_queue.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "storage/resource_mapper.hpp"
#include "transaction_store_sync_protocol.hpp"

#include <algorithm>
#include <unordered_map>

namespace fetch {
namespace ledger {

namespace transaction_store_sync_helper {
enum class State
{
  INITIAL = 0,
  QUERY_OBJECT_COUNTS,
  RESOLVING_OBJECT_COUNTS,
  QUERY_SUBTREE,
  RESOLVING_SUBTREE,
  QUERY_OBJECTS,
  RESOLVING_OBJECTS,
  TRIM_CACHE
};
}

class TransactionStoreSyncService
  : public network::AtomicStateMachine<transaction_store_sync_helper::State>,
    public VerifiedTransactionSink
{
public:
  using Muddle                = muddle::Muddle;
  using MuddlePtr             = std::shared_ptr<Muddle>;
  using Address               = Muddle::Address;
  using Uri                   = Muddle::Uri;
  using Client                = muddle::rpc::Client;
  using ClientPtr             = std::shared_ptr<Client>;
  using VerifiedTransaction   = chain::VerifiedTransaction;
  using ObjectStore           = storage::ObjectStore<VerifiedTransaction>;
  using FutureTimepoint       = network::FutureTimepoint;
  using UnverifiedTransaction = chain::UnverifiedTransaction;
  using RequestingObjectCount = network::RequestingQueueOf<Address, uint64_t>;
  using PromiseOfObjectCount  = network::PromiseOf<uint64_t>;
  using TxList                = std::vector<chain::UnverifiedTransaction>;
  using RequestingTxList      = network::RequestingQueueOf<Address, TxList>;
  using RequestingSubTreeList = network::RequestingQueueOf<uint64_t, TxList>;
  using PromiseOfTxList       = network::PromiseOf<TxList>;
  using ResourceID            = storage::ResourceID;
  using Mutex                 = mutex::Mutex;
  using EventNewTransaction   = std::function<void(VerifiedTransaction const &)>;
  using TrimCacheCallback     = std::function<void()>;
  using State                 = transaction_store_sync_helper::State;
  using ObjectStorePtr        = std::shared_ptr<ObjectStore>;
  using LaneControllerPtr     = std::shared_ptr<LaneController>;

  static constexpr char const *LOGGING_NAME = "TransactionStoreSyncService";
  static constexpr std::size_t MAX_OBJECT_COUNT_RESOLUTION_PER_CYCLE = 128;
  static constexpr std::size_t MAX_SUBTREE_RESOLUTION_PER_CYCLE      = 128;
  static constexpr std::size_t MAX_OBJECT_RESOLUTION_PER_CYCLE       = 128;
  static constexpr uint64_t PULL_LIMIT_ = 10000;  // Limit the amount a single rpc call will provide

  TransactionStoreSyncService(
      int lane_id, MuddlePtr muddle, ObjectStorePtr store, LaneControllerPtr controller_ptr,
      std::size_t               verification_threads,
      std::chrono::milliseconds the_timeout                = std::chrono::milliseconds(5000),
      std::chrono::milliseconds promise_wait_timeout       = std::chrono::milliseconds(2000),
      std::chrono::milliseconds fetch_object_wait_duration = std::chrono::milliseconds(5000))
    : muddle_(std::move(muddle))
    , store_(std::move(store))
    , verifier_(*this, verification_threads, "Lane " + std::to_string(lane_id) + ": ")
    , lane_controller_(std::move(controller_ptr))
    , time_duration_(the_timeout)
    , promise_wait_time_duration_(promise_wait_timeout)
    , fetch_object_wait_duration_(fetch_object_wait_duration)
    , id_(lane_id)
  {
    client_ = std::make_shared<Client>(muddle_->AsEndpoint(), Muddle::Address(), SERVICE_LANE,
                                       CHANNEL_RPC);

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

  virtual ~TransactionStoreSyncService()
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Lane ", id_, ": teardown sync service");
    muddle_->Shutdown();
    client_ = nullptr;
    muddle_ = nullptr;
    store_  = nullptr;
    FETCH_LOG_WARN(LOGGING_NAME, "Lane ", id_, ": sync service done!");
  }

  void Start()
  {
    verifier_.Start();
  }

  void Stop()
  {
    verifier_.Stop();
  }

  void SetTrimCacheCallback(TrimCacheCallback const &callback)
  {
    trim_cache_callback_ = callback;
  }

  virtual bool PossibleNewState(State &current_state) override
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
    switch (current_state)
    {
    default:
    case State::INITIAL:
    {
      return false;
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
      return true;
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
          return false;
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Lane ", id_, ": ",
                         "Still pending object count promises, but limit approached!");
        }
      }
      FETCH_LOG_INFO(LOGGING_NAME, "Lane ", id_, ": ", "Expected tx size: ", max_object_count_);

      // If there are objects to sync from the network, fetch N roots from each of the peers in
      // parallel. So if we decided to split the sync into 4 roots, the mask would be 2 (bits) and
      // the roots to sync 00, 10, 01 and 11...
      // where roots to sync are all objects with the key starting with those bits
      if (max_object_count_ != 0)
      {
        root_size_ = platform::Log2Ceil(((max_object_count_ / (PULL_LIMIT_ / 2)) + 1)) + 1;

        for (uint64_t i = 0, end = (1 << (root_size_)); i < end; ++i)
        {
          roots_to_sync_.push(Reverse(static_cast<uint8_t>(i)));
        }
      }
      current_state = State::QUERY_SUBTREE;
      return true;
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

        byte_array::ByteArray array;

        // TODO(unknown): This should be linked the a ResourceID size
        array.Resize(256 / 8);
        array[0] = root;

        auto promise                       = PromiseOfTxList(client_->CallSpecificAddress(
            connection, RPC_TX_STORE_SYNC, TransactionStoreSyncProtocol::PULL_SUBTREE, array,
            root_size_));
        promise_id_to_roots_[promise.id()] = root;
        pending_subtree_.Add(root, promise);
      }
      if (!roots_to_sync_.empty())
      {
        return false;
      }
      promise_wait_timeout_.Set(promise_wait_time_duration_);
      current_state = State::RESOLVING_SUBTREE;
      return true;
    }
    case State::RESOLVING_SUBTREE:
    {
      auto counts = pending_subtree_.Resolve();
      for (auto &result : pending_subtree_.Get(MAX_SUBTREE_RESOLUTION_PER_CYCLE))
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Lane ", id_, ": ", "Got ", result.promised.size(),
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
            return true;
          }
          return false;
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
      return true;
    }
    case State::QUERY_OBJECTS:
    {
      if (!fetch_object_wait_timeout_.IsDue())
      {
        return false;
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
      return true;
    }
    case State::RESOLVING_OBJECTS:
    {
      auto counts = pending_objects_.Resolve();
      for (auto &result : pending_objects_.Get(MAX_OBJECT_RESOLUTION_PER_CYCLE))
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Lane ", id_, ": ", "Got ", result.promised.size(),
                       " objects!");
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
          return false;
        }
        FETCH_LOG_WARN(LOGGING_NAME, "Lane ", id_, ": ",
                       "Still pending object promises but limit approached!");
      }
      FETCH_LOG_INFO(LOGGING_NAME, "Lane ", id_, ": ", "Failed promises: ", counts.failed);
      current_state = State::TRIM_CACHE;
      return true;
    }
    case State::TRIM_CACHE:
    {
      if (trim_cache_callback_)
      {
        trim_cache_callback_();
      }
      current_state = State::QUERY_OBJECTS;
      return true;
    }
    };
  }

  // We need this for the testing.
  bool IsReady()
  {
    FETCH_LOCK(is_ready_mutex_);
    return is_ready_;
  }

protected:
  void OnTransaction(chain::VerifiedTransaction const &tx) override
  {
    ResourceID const rid(tx.digest());

    if (!store_->Has(rid))
    {
      store_->Set(rid, tx);

#ifdef FETCH_ENABLE_METRICS
      RecordNewElement(tx.digest());
#endif  // FETCH_ENABLE_METRICS
    }
  }

  void OnTransactions(TransactionList const &txs) override
  {
    std::size_t batch_size  = 30;
    std::size_t num_batches = static_cast<std::size_t>(
        ceil(static_cast<double>(txs.size()) / static_cast<double>(batch_size)));
    for (std::size_t b = 0; b < num_batches; ++b)
    {
      store_->WithLock([this, &txs, &b, &batch_size]() {
        for (std::size_t i = 0, end = std::min(batch_size, txs.size() - b * batch_size); i < end;
             ++i)
        {
          auto const &     tx = txs[i + batch_size * b];
          ResourceID const rid(tx.digest());

          if (!store_->LocklessHas(rid))
          {
            store_->LocklessSet(rid, tx);

#ifdef FETCH_ENABLE_METRICS
            RecordNewElement(tx.digest());
#endif  // FETCH_ENABLE_METRICS
          }
        }
      });
    }
  }

  // Reverse bits in byte
  inline uint8_t Reverse(uint8_t c)
  {
    // https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
    return static_cast<uint8_t>(((c * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32);
  }

  inline void SetTimeOut()
  {
    if (timeout_set_)
    {
      return;
    }
    FETCH_LOG_INFO(LOGGING_NAME, "Lane ", id_, ": ", "Timeout set ", time_duration_.count());
    timeout_.Set(time_duration_);
    timeout_set_ = true;
  }

private:
  MuddlePtr           muddle_;
  ClientPtr           client_;
  ObjectStorePtr      store_;  ///< The pointer to the object store
  TransactionVerifier verifier_;

  LaneControllerPtr lane_controller_;

  std::chrono::milliseconds time_duration_;
  std::chrono::milliseconds promise_wait_time_duration_;
  FutureTimepoint           timeout_;
  FutureTimepoint           promise_wait_timeout_;
  bool                      timeout_set_ = false;
  std::chrono::milliseconds fetch_object_wait_duration_;
  FutureTimepoint           fetch_object_wait_timeout_;

  RequestingObjectCount pending_object_count_;
  uint64_t              max_object_count_;

  RequestingSubTreeList pending_subtree_;
  RequestingTxList      pending_objects_;

  std::queue<uint8_t>                                          roots_to_sync_;
  uint64_t                                                     root_size_ = 0;
  std::unordered_map<PromiseOfTxList::PromiseCounter, uint8_t> promise_id_to_roots_;

  TrimCacheCallback trim_cache_callback_;

  Mutex mutex_{__LINE__, __FILE__};

  int id_;

  Mutex is_ready_mutex_{__LINE__, __FILE__};
  bool  is_ready_ = false;
};

}  // namespace ledger
}  // namespace fetch