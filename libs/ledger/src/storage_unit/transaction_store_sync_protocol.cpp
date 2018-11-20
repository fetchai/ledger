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

#include "ledger/storage_unit/transaction_store_sync_protocol.hpp"
#include "ledger/chain/transaction_serialization.hpp"

using fetch::byte_array::ConstByteArray;
using fetch::storage::ResourceID;
using fetch::metrics::MetricHandler;
using fetch::metrics::Metrics;

#ifdef FETCH_ENABLE_METRICS
static void RecordNewElement(ConstByteArray const &identifier)
{
  // record the event
  Metrics::Instance().RecordMetric(identifier, MetricHandler::Instrument::TRANSACTION,
                                   MetricHandler::Event::SYNCED);
}

static void RecordNewCacheElement(ConstByteArray const &identifier)
{
  // record the event
  Metrics::Instance().RecordMetric(identifier, MetricHandler::Instrument::TRANSACTION,
                                   MetricHandler::Event::RECEIVED_FOR_SYNC);
}
#endif  // FETCH_ENABLE_METRICS

namespace fetch {
namespace ledger {

/**
 * Create a transaction store sync protocol
 *
 * @param p The protocol id for this service
 * @param r The register instance
 * @param tp The thread pool instance to be used
 * @param store The object store to be used
 */
TransactionStoreSyncProtocol::TransactionStoreSyncProtocol(ProtocolId const &p, Register r,
                                                           ThreadPool tp, ObjectStore &store, UnverifiedTransactionSink &sink)
  : fetch::service::Protocol()
  , protocol_(p)
  , register_(std::move(r))
  , thread_pool_(std::move(tp))
  , store_(store)
  , sink_(sink)
{
  this->Expose(OBJECT_COUNT, this, &Self::ObjectCount);
  this->ExposeWithClientArg(PULL_OBJECTS, this, &Self::PullObjects);
  this->Expose(PULL_SUBTREE, this, &Self::PullSubtree);
}

/**
 * Start the protocol running
 */
void TransactionStoreSyncProtocol::Start()
{
  if (running_)
  {
    return;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Starting transaction synchronisation");

  running_ = true;
  thread_pool_->Post([this]() { this->IdleUntilPeers(); });
}

/**
 * Stop the protocol from running
 */
void TransactionStoreSyncProtocol::Stop()
{
  running_ = false;
}

/**
 * Spin until the connected peers is adequate
 */
void TransactionStoreSyncProtocol::IdleUntilPeers()
{
  if (!running_)
  {
    return;
  }

  if (register_.number_of_services() == 0)
  {
    thread_pool_->Post([this]() { this->IdleUntilPeers(); },
                       1000);  // TODO(issue 9): Make time variable
  }
  else
  {
    // If we need to sync our object store (esp. when joining the network)
    if (needs_sync_)
    {
      thread_pool_->Post([this]() { this->SetupSync(); });
    }
    else
    {
      thread_pool_->Post([this]() { this->FetchObjectsFromPeers(); });
    }
  }
}

void TransactionStoreSyncProtocol::SetupSync()
{
  uint64_t obj_size = 0;

  // Determine the expected size of the obj store as the max of all peers
  register_.WithServices([this, &obj_size](ServiceMap const &map) {
    for (auto const &p : map)
    {
      if (!running_)
      {
        return;
      }

      auto                    peer    = p.second;
      auto                    ptr     = peer.lock();
      fetch::service::Promise promise = ptr->Call(protocol_, OBJECT_COUNT);

      uint64_t remote_size = promise->As<uint64_t>();

      obj_size = std::max(obj_size, remote_size);
    }
  });

  FETCH_LOG_INFO(LOGGING_NAME, "Expected tx size: ", obj_size);

  // If there are objects to sync from the network, fetch N roots from each of the peers in
  // parallel. So if we decided to split the sync into 4 roots, the mask would be 2 (bits) and
  // the roots to sync 00, 10, 01 and 11...
  // where roots to sync are all objects with the key starting with those bits
  if (obj_size != 0)
  {
    root_mask_ = platform::Log2Ceil(((obj_size / (PULL_LIMIT_ / 2)) + 1)) + 1;

    for (uint64_t i = 0, end = (1 << (root_mask_ + 1)); i < end; ++i)
    {
      roots_to_sync_.push(Reverse(static_cast<uint8_t>(i)));
    }
  }

  thread_pool_->Post([this]() { this->SyncSubtree(); });
}

void TransactionStoreSyncProtocol::FetchObjectsFromPeers()
{
  if (!running_)
  {
    return;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Fetching objects from peer");

  generics::MilliTimer lock_timer("ObjectSync:FetchObjectsFromPeers");
  FETCH_LOCK(object_list_mutex_);

  register_.WithServices([this](ServiceMap const &map) {
    for (auto const &p : map)
    {
      if (!running_)
      {
        return;
      }

      auto peer = p.second;
      auto ptr  = peer.lock();
      object_list_promises_.push_back(ptr->Call(protocol_, PULL_OBJECTS));
    }
  });

  if (running_)
  {
    thread_pool_->Post([this]() { this->RealisePromises(); });
  }
}

void TransactionStoreSyncProtocol::RealisePromises(std::size_t index)
{
  if (!running_)
  {
    return;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Waiting for promise backlog index: ", index);

  generics::MilliTimer lock_timer("ObjectSync:RealisePromises", 10);
  FETCH_LOCK(object_list_mutex_);

  // reserve the elements required for the cache elements

  FETCH_LOG_INFO(LOGGING_NAME, "Promise backlog before: ", object_list_promises_.size());

  auto it = object_list_promises_.begin();
  while (it != object_list_promises_.end())
  {
    auto &p = *it;

    if (!running_)
    {
      return;
    }

    TxList incoming_objects;
    incoming_objects.reserve(MAX_CACHE_ELEMENTS);

    FETCH_LOG_PROMISE();
    if (!p->Wait(1000, false))
    {
      ++it;
      continue;
    }

    // retrieve the transaction list from the peer
    p->As<TxList>(incoming_objects);

    if (!running_)
    {
      return;
    }

    if (!incoming_objects.empty())
    {
      for (auto const &tx : incoming_objects)
      {
        if (!store_.Has(storage::ResourceID{tx.digest()}))
        {
          FETCH_LOG_INFO(LOGGING_NAME, "Oh look a 'new' tx: ", byte_array::ToBase64(tx.digest()));

          // dispatch the transaction to the new store
          sink_.OnTransaction(tx);

#ifdef FETCH_ENABLE_METRICS
          RecordNewElement(tx.digest());
#endif // FETCH_ENABLE_METRICS
        }
      }
    }

    // remove the entry
    it = object_list_promises_.erase(it);
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Promise backlog after: ", object_list_promises_.size());

  if (object_list_promises_.empty())
  {
    if (running_)
    {
      thread_pool_->Post([this]() { this->TrimCache(); });
    }
  }
  else
  {
    if (running_)
    {
      thread_pool_->Post([this, index]() { this->RealisePromises(index + 1); });
    }
  }
}

void TransactionStoreSyncProtocol::TrimCache()
{
  {
    generics::MilliTimer timer("ObjectSync:TrimCache", 10);
    FETCH_LOCK(cache_mutex_);

    // reserve the space for the next cache
    Cache next_cache;
    next_cache.reserve(cache_.size()); // worst case

    // compute the deadline for the cache entries
    auto const cut_off = CachedObject::Clock::now() - std::chrono::milliseconds(uint32_t{MAX_CACHE_LIFETIME_MS});

    // generate the next cache version
    std::copy_if(
      cache_.begin(),
      cache_.end(),
      std::back_inserter(next_cache),
      [&cut_off](CachedObject const &object) {

        // we only copy objects which are young that we want to keep
        return object.created_ > cut_off;
      }
    );

    FETCH_LOG_INFO(LOGGING_NAME, "New cache size: ", next_cache.size(), " Old cache size: ", cache_.size());

    // replace the old cache
    cache_ = std::move(next_cache);
  }

  if (running_)
  {
    // TODO(issue 9): Make time parameter
    thread_pool_->Post([this]() { this->IdleUntilPeers(); }, 5000);
  }
}

/// @}

void TransactionStoreSyncProtocol::OnNewTx(VerifiedTransaction const &o)
{
#ifdef FETCH_ENABLE_METRICS
  RecordNewCacheElement(o.digest());
#endif  // FETCH_ENABLE_METRICS

  FETCH_LOCK(cache_mutex_);
  cache_.emplace_back(o);
}

/**
 * Allow peers to pull large sections of your subtree for synchronisation on entry to the network
 *
 * @param: client_handle Handle referencing client making the request
 *
 * @return: the subtree the client is requesting as a vector (size limited)
 */
TransactionStoreSyncProtocol::TxList TransactionStoreSyncProtocol::PullSubtree(byte_array::ConstByteArray const &rid, uint64_t mask)
{
  TxList ret;

  uint64_t counter = 0;

  store_.WithLock([this, &ret, &counter, &rid, mask]() {
    // This is effectively saying get all objects whose ID begins rid & mask
    auto it = store_.GetSubtree(ResourceID(rid), mask);

    while (it != store_.end() && counter++ < PULL_LIMIT_)
    {
      ret.push_back(*it);
      ++it;
    }
  });

  return ret;
}

//void TransactionStoreSyncProtocol::StartSync()
//{
//  needs_sync_ = true;
//}
//
//bool TransactionStoreSyncProtocol::FinishedSync()
//{
//  return !needs_sync_;
//}

uint64_t TransactionStoreSyncProtocol::ObjectCount()
{
  FETCH_LOCK(cache_mutex_);
  return store_.size();
}

TransactionStoreSyncProtocol::TxList TransactionStoreSyncProtocol::PullObjects(uint64_t const &client_handle)
{
  // Creating result
  TxList ret;

  {
    generics::MilliTimer timer("ObjectSync:PullObjects");
    FETCH_LOCK(cache_mutex_);

    if (!cache_.empty())
    {
      for (auto &c : cache_)
      {
        if (c.delivered_to.find(client_handle) == c.delivered_to.end())
        {
          c.delivered_to.insert(client_handle);
          ret.push_back(c.data);
        }
      }
    }
  }

  return ret;
}

// Create a stack of subtrees we want to sync. Push roots back onto this when the promise
// fails. Completion when the stack is empty
void TransactionStoreSyncProtocol::SyncSubtree()
{
  if (!running_)
  {
    return;
  }

  register_.WithServices([this](ServiceMap const &map) {
    for (auto const &p : map)
    {
      if (!running_)
      {
        return;
      }

      auto peer = p.second;
      auto ptr  = peer.lock();

      if (roots_to_sync_.empty())
      {
        break;
      }

      auto root = roots_to_sync_.front();
      roots_to_sync_.pop();

      byte_array::ByteArray array;

      // TODO: This should be linked the a ResourceID size
      array.Resize(256 / 8);
      array[0] = root;

      auto promise = ptr->Call(protocol_, PULL_SUBTREE, array, root_mask_);
      subtree_promises_.emplace_back(std::make_pair(root, std::move(promise)));
    }
  });

  thread_pool_->Post([this]() { this->RealiseSubtreePromises(); }, 200);
}

/**
 * Wait for the subtree promised to completed
 */
void TransactionStoreSyncProtocol::RealiseSubtreePromises()
{
  for (auto &promise_pair : subtree_promises_)
  {
    auto &root    = promise_pair.first;
    auto &promise = promise_pair.second;

    // Timeout fail, push this subtree back onto the root for another go
    if (!promise->Wait(1000, false))
    {
      roots_to_sync_.push(root);
      continue;
    }

    // retrieve the incoming transctions from the peer
    TxList incoming_txs;
    promise->As<TxList>(incoming_txs);

    // dispatch the transaction to the transaction sink
    for (auto &tx : incoming_txs)
    {
      if (!store_.Has(storage::ResourceID{tx.digest()}))
      {
        // dispatch the transaction to the new store
        sink_.OnTransaction(tx);

#ifdef FETCH_ENABLE_METRICS
        RecordNewElement(tx.digest());
#endif // FETCH_ENABLE_METRICS
      }
    }
  }

  // clear the promises out ("failed" attempts are stored in `roots_to_sync`)
  subtree_promises_.clear();

  // Completed syncing
  if (roots_to_sync_.empty())
  {
    needs_sync_ = false;
    thread_pool_->Post([this]() { this->IdleUntilPeers(); });
  }
  else
  {
    thread_pool_->Post([this]() { this->SyncSubtree(); });
  }
}


} // namespace ledger
} // namespace fetch
