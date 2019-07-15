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

#include "ledger/chain/transaction_rpc_serializers.hpp"
#include "ledger/storage_unit/transaction_store_sync_protocol.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

using fetch::byte_array::ConstByteArray;

// TODO(issue 7): Make cache configurable
constexpr uint32_t MAX_CACHE_LIFETIME_MS = 20000;

#ifdef FETCH_ENABLE_METRICS
using fetch::metrics::Metrics;
using fetch::metrics::MetricHandler;

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
TransactionStoreSyncProtocol::TransactionStoreSyncProtocol(ObjectStore *store, int lane_id)
  : fetch::service::Protocol()
  , store_(store)
  , id_(lane_id)
{
  this->Expose(OBJECT_COUNT, this, &Self::ObjectCount);
  this->ExposeWithClientContext(PULL_OBJECTS, this, &Self::PullObjects);
  this->Expose(PULL_SUBTREE, this, &Self::PullSubtree);
  this->Expose(PULL_SPECIFIC_OBJECTS, this, &Self::PullSpecificObjects);
}

void TransactionStoreSyncProtocol::TrimCache()
{
  generics::MilliTimer timer("ObjectSync:TrimCache", 500);
  FETCH_LOCK(cache_mutex_);

  // reserve the space for the next cache
  Cache next_cache;
  next_cache.reserve(cache_.size());  // worst case

  // compute the deadline for the cache entries
  auto const cut_off =
      CachedObject::Clock::now() - std::chrono::milliseconds(MAX_CACHE_LIFETIME_MS);

  // generate the next cache version
  std::copy_if(cache_.begin(), cache_.end(), std::back_inserter(next_cache),
               [&cut_off](CachedObject const &object) {
                 // we only copy objects which are young that we want to keep
                 return object.created > cut_off;
               });

  auto const next_cache_size = next_cache.size();
  auto const curr_cache_size = cache_.size();

  if (curr_cache_size && (next_cache_size != curr_cache_size))
  {
    FETCH_UNUSED(id_);  // logging only
    FETCH_LOG_DEBUG(LOGGING_NAME, "Lane ", id_, ": New cache size: ", next_cache_size,
                    " Old cache size: ", curr_cache_size);
  }

  // replace the old cache
  cache_ = std::move(next_cache);
}

/// @}

void TransactionStoreSyncProtocol::OnNewTx(Transaction const &o)
{
#ifdef FETCH_ENABLE_METRICS
  RecordNewCacheElement(o.digest());
#endif  // FETCH_ENABLE_METRICS

  FETCH_LOCK(cache_mutex_);
  cache_.emplace_back(o);
}

uint64_t TransactionStoreSyncProtocol::ObjectCount()
{
  FETCH_LOCK(cache_mutex_);

  return store_->Size();
}

/**
 * Allow peers to pull large sections of your subtree for synchronisation on entry to the network
 *
 * @param: client_handle Handle referencing client making the request
 *
 * @return: the subtree the client is requesting as a vector (size limited)
 */
TransactionStoreSyncProtocol::TxArray TransactionStoreSyncProtocol::PullSubtree(
    byte_array::ConstByteArray const &rid, uint64_t bit_count)
{
  return store_->PullSubtree(rid, bit_count, PULL_LIMIT_);
}

TransactionStoreSyncProtocol::TxArray TransactionStoreSyncProtocol::PullObjects(
    service::CallContext const *call_context)
{
  // Creating result
  TxArray ret{};

  {
    generics::MilliTimer timer("ObjectSync:PullObjects", 500);
    FETCH_LOCK(cache_mutex_);

    if (!cache_.empty())
    {
      for (auto &c : cache_)
      {
        if (c.delivered_to.find(call_context->sender_address) == c.delivered_to.end())
        {
          c.delivered_to.insert(call_context->sender_address);
          ret.push_back(c.data);
        }
      }
    }
  }

  return ret;
}

TransactionStoreSyncProtocol::TxArray TransactionStoreSyncProtocol::PullSpecificObjects(
    std::vector<storage::ResourceID> const &rids)
{
  TxArray     ret;
  Transaction tx;

  for (auto const &rid : rids)
  {
    if (store_->Get(rid, tx))
    {
      ret.push_back(tx);
    }
  }

  return ret;
}

}  // namespace ledger
}  // namespace fetch
