#pragma once
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

#include "chain/transaction.hpp"
#include "core/digest.hpp"
#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "ledger/storage_unit/transaction_sinks.hpp"
#include "ledger/transaction_verifier.hpp"
#include "logging/logging.hpp"
#include "muddle/address.hpp"
#include "network/details/thread_pool.hpp"
#include "network/generics/milli_timer.hpp"
#include "network/service/call_context.hpp"
#include "network/service/promise.hpp"
#include "network/service/protocol.hpp"
#include "storage/resource_mapper.hpp"
#include "vectorise/platform.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace fetch {
namespace ledger {

class TransactionStorageEngineInterface;

class TransactionStoreSyncProtocol : public fetch::service::Protocol
{
public:
  enum
  {
    OBJECT_COUNT          = 1,
    PULL_OBJECTS          = 2,
    PULL_SUBTREE          = 3,
    PULL_SPECIFIC_OBJECTS = 4
  };

  static constexpr char const *LOGGING_NAME = "ObjectStoreSyncProtocol";

  // Construction / Destruction
  TransactionStoreSyncProtocol(TransactionStorageEngineInterface &store, uint32_t lane_id);
  TransactionStoreSyncProtocol(TransactionStoreSyncProtocol const &) = delete;
  TransactionStoreSyncProtocol(TransactionStoreSyncProtocol &&)      = delete;
  ~TransactionStoreSyncProtocol() override                           = default;

  void OnNewTx(chain::Transaction const &tx);
  void TrimCache();

  // Operators
  TransactionStoreSyncProtocol &operator=(TransactionStoreSyncProtocol const &) = delete;
  TransactionStoreSyncProtocol &operator=(TransactionStoreSyncProtocol &&) = delete;

private:
  // Limit the amount a single rpc call will provide
  static constexpr uint64_t PULL_LIMIT = 10000u;

  struct CachedObject
  {
    using Clock      = std::chrono::system_clock;
    using Timepoint  = Clock::time_point;
    using AddressSet = std::unordered_set<muddle::Address>;

    explicit CachedObject(chain::Transaction value)
      : data(std::move(value))
    {}

    chain::Transaction data;
    Timepoint          created{Clock::now()};
  };

  using Cache   = std::vector<CachedObject>;
  using TxArray = std::vector<chain::Transaction>;
  using TxStore = TransactionStorageEngineInterface;

  uint64_t ObjectCount();
  TxArray  PullObjects(service::CallContext const &call_context);
  TxArray  PullSubtree(byte_array::ConstByteArray const &rid, uint64_t bit_count);
  TxArray  PullSpecificObjects(DigestSet const &digests);

  telemetry::CounterPtr   CreateCounter(char const *operation) const;
  telemetry::HistogramPtr CreateHistogram(char const *operation) const;

  uint32_t const lane_;
  TxStore &      store_;  ///< The pointer to the object store

  Mutex cache_mutex_;  ///< The mutex protecting cache_
  Cache cache_;

  // telemetry
  telemetry::CounterPtr   object_count_total_;
  telemetry::CounterPtr   pull_objects_total_;
  telemetry::CounterPtr   pull_subtree_total_;
  telemetry::CounterPtr   pull_specific_objects_total_;
  telemetry::HistogramPtr object_count_durations_;
  telemetry::HistogramPtr pull_objects_durations_;
  telemetry::HistogramPtr pull_subtree_durations_;
  telemetry::HistogramPtr pull_specific_objects_durations_;
};

}  // namespace ledger
}  // namespace fetch
