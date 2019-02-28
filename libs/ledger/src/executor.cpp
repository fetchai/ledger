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

// Useful to include when debugging:
//
// std::ostream &operator<<(std::ostream &stream, Executor::lane_set_type const &lane_set)
//{
//  std::vector<uint16_t> elements(lane_set.size());
//  std::copy(lane_set.begin(), lane_set.end(), elements.begin());
//  std::sort(elements.begin(), elements.end());
//
//  bool not_first_loop = false;
//  for (auto element : elements)
//  {
//    if (not_first_loop)
//      stream << ',';
//    stream << element;
//    not_first_loop = true;
//  }
//  return stream;
//}

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
    FETCH_LOG_ERROR(LOGGING_NAME, "Unable to do full retrieve of TX: ", byte_array::ToBase64(hash));
    return Status::TX_LOOKUP_FAILURE;
  }

  Identifier id;
  id.Parse(tx.contract_name());

  std::vector<storage::ResourceAddress> successfully_locked_resources;

  auto deleter = [&](uint16_t *) {
    for (auto const &resource : successfully_locked_resources)
    {
      resources_->Unlock(resource);
    }
  };

  std::unique_ptr<uint16_t, decltype(deleter)> on_function_exit((uint16_t *)nullptr, deleter);

  // Lock raw resources for SC access
  for (auto const &hash : tx.contract_hashes())
  {
    storage::ResourceAddress address(hash);

    if (!resources_->Lock(address))
    {
      return Status::RESOURCE_FAILURE;
    }

    successfully_locked_resources.push_back(std::move(address));
  }

  // Lock wrapped resources for contract data
  for (auto const &resource : tx.resources())
  {
    storage::ResourceAddress address(CreateStateIndexWrapped(id[0], id[1], resource));

    if (!resources_->Lock(address))
    {
      return Status::RESOURCE_FAILURE;
    }

    successfully_locked_resources.push_back(std::move(address));
  }

  // Lookup the chain code associated with the transaction
  auto chain_code = chain_code_cache_.Lookup(id.name_space(), resources_.get());

  if (!chain_code)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Chain code lookup failure!");
    return Status::CHAIN_CODE_LOOKUP_FAILURE;
  }

  // attach the chain code to the current working context
  chain_code->Attach(*resources_);

  // Dispatch the transaction to the contract
  FETCH_LOG_INFO(LOGGING_NAME, "Dispatch: ", id.name());

  auto result = chain_code->DispatchTransaction(id.name(), tx, nullptr);

  if (Contract::Status::OK != result)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to dispatch transaction!");
    return Status::CHAIN_CODE_EXEC_FAILURE;
  }

  // detach the chain code from the current context
  chain_code->Detach();

#ifdef FETCH_ENABLE_METRICS
  Metrics::Timestamp const completed = Metrics::Clock::now();
#endif  // FETCH_ENABLE_METRICS

  FETCH_LOG_DEBUG(LOGGING_NAME, "Executing tx ", byte_array::ToBase64(hash), " (success)");

  FETCH_METRIC_TX_EXEC_STARTED_EX(hash, started);
  FETCH_METRIC_TX_EXEC_COMPLETE_EX(hash, completed);

  return Status::SUCCESS;
}

}  // namespace ledger
}  // namespace fetch
