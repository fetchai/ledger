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
#include "ledger/storage_unit/transaction_storage_engine_interface.hpp"
#include "ledger/storage_unit/transaction_store_sync_protocol.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/utils/timer.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace fetch {
namespace ledger {
namespace {

// TODO(issue 7): Make cache configurable
constexpr uint32_t MAX_CACHE_LIFETIME_MS = 60 * 1000;

using fetch::byte_array::ConstByteArray;

using TSSP   = TransactionStoreSyncProtocol;
using Labels = fetch::telemetry::Measurement::Labels;

}  // namespace

/**
 * Create a transaction store sync protocol
 *
 * @param p The protocol id for this service
 * @param r The register instance
 * @param tp The thread pool instance to be used
 * @param store The object store to be used
 */
TransactionStoreSyncProtocol::TransactionStoreSyncProtocol(TransactionStorageEngineInterface &store,
                                                           uint32_t lane_id)
  : lane_(lane_id)
  , store_(store)
  , object_count_total_{CreateCounter("object_count")}
  , pull_objects_total_{CreateCounter("pull_objects")}
  , pull_subtree_total_{CreateCounter("pull_subtree")}
  , pull_specific_objects_total_{CreateCounter("pull_specific")}
  , object_count_durations_{CreateHistogram("object_count")}
  , pull_objects_durations_{CreateHistogram("pull_objects")}
  , pull_subtree_durations_{CreateHistogram("pull_subtree")}
  , pull_specific_objects_durations_{CreateHistogram("pull_specific")}
{
  Expose(OBJECT_COUNT, this, &TransactionStoreSyncProtocol::ObjectCount);
  ExposeWithClientContext(PULL_OBJECTS, this, &TransactionStoreSyncProtocol::PullObjects);
  Expose(PULL_SUBTREE, this, &TransactionStoreSyncProtocol::PullSubtree);
  Expose(PULL_SPECIFIC_OBJECTS, this, &TransactionStoreSyncProtocol::PullSpecificObjects);
}

/**
 * Add a new transaction to the recently seen cache
 *
 * @param tx The transaction that has been seen
 */
void TransactionStoreSyncProtocol::OnNewTx(chain::Transaction const &tx)
{
  FETCH_LOCK(cache_mutex_);
  cache_.emplace_back(tx);
}

/**
 * Trims the current cache based on the time present in the cache
 */
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

  if ((curr_cache_size != 0u) && (next_cache_size != curr_cache_size))
  {
    FETCH_LOG_VARIABLE(lane_);
    FETCH_LOG_DEBUG(LOGGING_NAME, "Lane ", lane_, ": New cache size: ", next_cache_size,
                    " Old cache size: ", curr_cache_size);
  }

  // replace the old cache
  cache_ = std::move(next_cache);
}

/**
 * Query the count of transactiobs that are available
 *
 * @return The number of transaction that are available
 */
uint64_t TransactionStoreSyncProtocol::ObjectCount()
{
  object_count_total_->increment();
  telemetry::FunctionTimer telemetry_timer{*object_count_durations_};
  return store_.GetCount();
}

/**
 * Pull recent transaction objects from peer
 *
 * This is the normal transaction synchronisation mechanism
 *
 * @param call_context The calling context of the RPC call
 * @return The new transactions to sync
 */
TSSP::TxArray TransactionStoreSyncProtocol::PullObjects(service::CallContext const &call_context)
{
  FETCH_UNUSED(call_context);

  pull_objects_total_->increment();

  // Creating result
  TxArray ret{};

  {
    telemetry::FunctionTimer telemetry_timer{*pull_objects_durations_};
    generics::MilliTimer     timer("ObjectSync:PullObjects", 500);
    FETCH_LOCK(cache_mutex_);

    if (!cache_.empty())
    {
      for (auto &c : cache_)
      {
        ret.push_back(c.data);
      }
    }
  }

  if (!ret.empty())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Lane ", lane_, ": PullObjects: Sending back ", ret.size(),
                   " TXs");
  }

  return ret;
}

/**
 * Allow peers to pull large sections of your subtree for synchronisation on entry to the network
 *
 * @param: client_handle Handle referencing client making the request
 *
 * @return: the subtree the client is requesting as a vector (size limited)
 */
TSSP::TxArray TransactionStoreSyncProtocol::PullSubtree(byte_array::ConstByteArray const &rid,
                                                        uint64_t                          bit_count)
{
  pull_subtree_total_->increment();

  telemetry::FunctionTimer telemetry_timer{*pull_subtree_durations_};
  generics::MilliTimer     timer("ObjectSync:PullSubtree", 500);

  return store_.PullSubtree(rid, bit_count, PULL_LIMIT);
}

/**
 * Pull a specific set of transaction digest from the shard
 *
 * @param digests The set of transactions to search for
 * @return The found transactions
 */
TSSP::TxArray TransactionStoreSyncProtocol::PullSpecificObjects(DigestSet const &digests)
{
  pull_specific_objects_total_->increment();

  telemetry::FunctionTimer telemetry_timer{*pull_specific_objects_durations_};
  generics::MilliTimer     timer("ObjectSync:PullSpecificObjects", 500);

  TxArray            ret;
  chain::Transaction tx;

  for (auto const &digest : digests)
  {
    if (store_.Get(digest, tx))
    {
      ret.push_back(tx);
    }
  }

  return ret;
}

/**
 * Create a total metric for a specified operation
 *
 * @param operation The operation
 * @return The generated counter
 */
telemetry::CounterPtr TransactionStoreSyncProtocol::CreateCounter(char const *operation) const
{
  std::ostringstream name, description;
  name << "ledger_tx_sync_store_" << operation << "_total";
  description << "The total number of '" << operation << "' operations";

  Labels const labels{{"lane", std::to_string(lane_)}};

  return telemetry::Registry::Instance().CreateCounter(name.str(), description.str(), labels);
}

/**
 * Create a duration histogram for a specified operation
 *
 * @param operation The operation
 * @return The generated histogram
 */
telemetry::HistogramPtr TransactionStoreSyncProtocol::CreateHistogram(char const *operation) const
{
  std::ostringstream name, description;
  name << "ledger_tx_sync_store_" << operation << "_duration";
  description << "The histogram of '" << operation << "' operation durations in seconds";

  Labels const labels{{"lane", std::to_string(lane_)}};

  return telemetry::Registry::Instance().CreateHistogram(
      {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
       0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
       0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
       0.001,    0.01,     0.1,      1,        2,        3,        4,        5,        6,
       7,        8,        9,        10.,      20.,      30.,      40.,      50.,      60.,
       70.,      80.,      90.,      100.},
      name.str(), description.str(), labels);
}

}  // namespace ledger
}  // namespace fetch
