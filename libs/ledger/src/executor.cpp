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

#include "ledger/executor.hpp"
#include "core/assert.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/logger.hpp"
#include "core/macros.hpp"
#include "core/mutex.hpp"
#include "ledger/chaincode/contract.hpp"
#include "ledger/state_sentinel.hpp"
#include "ledger/storage_unit/cached_storage_adapter.hpp"
#include "metrics/metrics.hpp"

#include <algorithm>
#include <chrono>
#include <random>
#include <thread>

static constexpr char const *LOGGING_NAME = "Executor";

#ifdef FETCH_ENABLE_METRICS
using fetch::metrics::Metrics;
#endif  // FETCH_ENABLE_METRICS

namespace fetch {
namespace ledger {

/**
 * Executes a given transaction across a series of lanes
 *
 * @param hash The transaction hash
 * @param slice The current block slice
 * @param lanes The affected lanes for the transaction
 * @return The status code for the operation
 */
Executor::Status Executor::Execute(TxDigest const &hash, std::size_t slice, LaneSet const &lanes)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Executing tx ", byte_array::ToBase64(hash));

  try
  {
    // TODO(issue 33): Add code to validate / check lane resources
    FETCH_UNUSED(slice);
    FETCH_UNUSED(lanes);

#ifdef FETCH_ENABLE_METRICS
    Metrics::Timestamp const started = Metrics::Clock::now();
#endif  // FETCH_ENABLE_METRICS

    // Get the transaction from the store
    Transaction tx;
    if (!resources_->GetTransaction(hash, tx))
    {
      return Status::TX_LOOKUP_FAILURE;
    }

    // This is a failure case that appears too often
    if (tx.contract_name().size() == 0)
    {
      FETCH_LOG_ERROR(LOGGING_NAME,
                      "Unable to do full retrieve of TX: ", byte_array::ToBase64(hash));
      return Status::TX_LOOKUP_FAILURE;
    }

    // attempt to parse the full contract name from from the transaction
    Identifier contract;
    if (!contract.Parse(tx.contract_name()))
    {
      return Status::CONTRACT_NAME_PARSE_FAILURE;
    }

    Contract::Status result{Contract::Status::FAILED};
    {
      // create the cache and state sentinel (lock and unlock resources as well as sandbox)
      //CachedStorageAdapter storage_cache{*resources_}; // TODO(HUT): rename here - confusing.
      StateSentinelAdapter storage_adapter{*resources_, contract.GetParent(), tx.resources()};

      // lookup or create the instance of the contract as is needed
      auto chain_code = chain_code_cache_.Lookup(contract.GetParent(), *resources_);
      if (!chain_code)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Chain code lookup failure!");
        return Status::CHAIN_CODE_LOOKUP_FAILURE;
      }

      // attach the chain code to the current working context
      chain_code->Attach(storage_adapter);

      // Dispatch the transaction to the contract
      FETCH_LOG_INFO(LOGGING_NAME, "Dispatch: ", contract.name());
      result = chain_code->DispatchTransaction(contract.name(), tx);

      // detach the chain code from the current context
      chain_code->Detach();

      // force the flushing of the cache
      //storage_cache.Flush();
    }

    // map the dispatch the result
    Status status{Status::CHAIN_CODE_EXEC_FAILURE};
    switch (result)
    {
    case Contract::Status::OK:
      status = Status::SUCCESS;
      break;
    case Contract::Status::FAILED:
      FETCH_LOG_WARN(LOGGING_NAME, "Transaction execution failed!");
      break;
    case Contract::Status::NOT_FOUND:
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup transaction handler");
      break;
    }

#ifdef FETCH_ENABLE_METRICS
    Metrics::Timestamp const completed = Metrics::Clock::now();
#endif  // FETCH_ENABLE_METRICS

    FETCH_LOG_DEBUG(LOGGING_NAME, "Executing tx ", byte_array::ToBase64(hash), " (success)");

    FETCH_METRIC_TX_EXEC_STARTED_EX(hash, started);
    FETCH_METRIC_TX_EXEC_COMPLETE_EX(hash, completed)

    return status;
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Execution Error: ", ex.what());
  }

  return Status::INEXPLICABLE_FAILURE;
}

}  // namespace ledger
}  // namespace fetch
