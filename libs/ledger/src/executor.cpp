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
#include "ledger/storage_unit/cached_storage_adapter.hpp"
#include "metrics/metrics.hpp"
#include "ledger/chain/v2/transaction.hpp"
#include "ledger/chaincode/token_contract.hpp"

#include "ledger/state_sentinel_adapter.hpp"

#include <algorithm>
#include <chrono>
#include <random>
#include <thread>

static constexpr char const *LOGGING_NAME = "Executor";

using fetch::byte_array::ConstByteArray;

#ifdef FETCH_ENABLE_METRICS
using fetch::metrics::Metrics;
#endif  // FETCH_ENABLE_METRICS

namespace fetch {
namespace ledger {
namespace {

  bool GenerateContractName(v2::Transaction const &tx, Identifier &identifier)
  {
    // Step 1 - Translate the tx into a common name
    using ContractMode = v2::Transaction::ContractMode;

    ConstByteArray contract_name{};
    switch (tx.contract_mode())
    {
      case ContractMode::NOT_PRESENT:
        break;
      case ContractMode::PRESENT:
        contract_name = tx.contract_digest().address().ToHex() + "." + tx.contract_address().address().ToHex() + "." + tx.action();
        break;
      case ContractMode::CHAIN_CODE:
        contract_name = tx.chain_code() + "." + tx.action();
        break;
    }

    // if there is a contract present simply parse the name
    if (!contract_name.empty())
    {
      if (!identifier.Parse(contract_name))
      {
        return false;
      }
    }

    return true;
  }

//  ExecutorStatus ExecuteContract(ContractStatus &contract_status,
//                                 ConstByteArray const &name,
//                                 StorageUnitPtr const &storage,
//                                 ChainCodeCache &chain_code_cache,
//                                 LaneIndex log2_num_lanes, LaneSet const &lanes)
//  {
//

}

Executor::Executor(StorageUnitPtr storage)
  : storage_{std::move(storage)}
  , token_contract_{std::make_shared<TokenContract>()}
{}

/**
 * Executes a given transaction across a series of lanes
 *
 * @param hash The transaction hash
 * @param slice The current block slice
 * @param lanes The affected lanes for the transaction
 * @return The status code for the operation
 */
Executor::Result Executor::Execute(TxDigest const &digest, LaneIndex log2_num_lanes, LaneSet lanes)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Executing tx ", byte_array::ToBase64(hash));

  Result result{Status::INEXPLICABLE_FAILURE, 0u};

  // cache the state for the current transaction
  allowed_lanes_ = std::move(lanes);
  log2_num_lanes_ = log2_num_lanes;

  // attempt to retrieve the transaction from the storage
  if (!RetrieveTransaction(digest))
  {
    // signal that the contract failed to be executed
    result.status = Status::TX_LOOKUP_FAILURE;
  }
  else
  {
    // create the storage cache
    storage_cache_ = std::make_shared<CachedStorageAdapter>(*storage_);

    // execute the transaction
    if (!ExecuteTransactionContract(result))
    {
      // in the case of transaction execution failure the maximum fees will be removed
      result.fee = current_tx_->charge_limit();

      // in addition to avoid indeterminate data being partially flushed. In the case of the when
      // the transaction execution fails then we also clear ally the cached version.
      storage_cache_->Clear();
    }

#if 0
    // process the fees
    if (!ProcessFees(result.fee))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to process the fees");
      result.status = Status::CHAIN_CODE_EXEC_FAILURE;
    }
#endif

    // force the flushing of the cache *** only if the contract was successful ***
    if (Executor::Status::SUCCESS == result.status)
    {
      storage_cache_->Flush();
    }
  }

  // clean up any used resources
  Cleanup();

  return result;
}

bool Executor::RetrieveTransaction(TxDigest const &hash)
{
  bool success{false};

  try
  {
    // create a new transaction
    current_tx_ = std::make_unique<v2::Transaction>();

    // load the transaction from the store
    success = storage_->GetTransaction(hash, *current_tx_);
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Exception caught when retrieving tx from store: ", ex.what());
  }

  return success;
}

bool Executor::ExecuteTransactionContract(Result &result)
{
  bool success{false};

  try
  {
    Identifier contract{};

    // generate the contract name (identifier)
    if (!GenerateContractName(*current_tx_, contract))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to generate the contract name");
      result.status = Status::CONTRACT_NAME_PARSE_FAILURE;
      return false;
    }

    // when there is no contract signalled in the transaction the identifier will be empty. This is
    // a normal use case
    if (contract.empty())
    {
      result.status = Status::SUCCESS;
      return true;
    }

    // create the cache and state sentinel (lock and unlock resources as well as sandbox)
    StateSentinelAdapter storage_adapter{*storage_cache_, contract.GetParent(), log2_num_lanes_,
                                         allowed_lanes_};

    // lookup or create the instance of the contract as is needed
    auto chain_code = chain_code_cache_.Lookup(contract.GetParent(), *storage_);
    if (!chain_code)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Chain code lookup failure!");
      result.status = Status::CONTRACT_LOOKUP_FAILURE;
    }

    // attach the chain code to the current working context
    chain_code->Attach(storage_adapter);

    // Dispatch the transaction to the contract
    FETCH_LOG_DEBUG(LOGGING_NAME, "Dispatch: ", contract.name());
    auto const contract_status = chain_code->DispatchTransaction(contract.name(), *current_tx_);

    // detach the chain code from the current context
    chain_code->Detach();

    // map the contract execution status
    result.status = Status::CHAIN_CODE_EXEC_FAILURE;
    switch (contract_status)
    {
      case Contract::Status::OK:
        success = true;
        result.status  = Status::SUCCESS;
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
    FETCH_METRIC_TX_EXEC_COMPLETE_EX(hash, completed);

    // attempt to generate a fee for this transaction
    if (success)
    {
      // simple linear scale fee
      uint64_t const compute_fee = chain_code->CalculateFee();
      uint64_t const storage_fee = (storage_adapter.num_bytes_written() * 2u);
      uint64_t const base_fee    = compute_fee + storage_fee;
      uint64_t const scaled_fee  = allowed_lanes_.size() * base_fee;

      result.fee = scaled_fee;
    }
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Exception during execution of tx 0x", current_tx_->digest().ToHex(), " : ", ex.what());

    result.status = Status::CHAIN_CODE_EXEC_FAILURE;
  }

  return success;
}

bool Executor::HandleTokenUpdates(uint64_t &fee)
{
  // create the token state adapter
  StateSentinelAdapter storage_adapter{*storage_cache_, Identifier{"fetch.token"}, log2_num_lanes_,
                                       allowed_lanes_};

#if 0
  // attach the token contract
  token_contract_->Attach(storage_adapter);

//  token_contract_->Transfer()

  // detach the token contract
  token_contract_->Detach();
#endif

  return false;
}

bool Executor::Cleanup()
{
  return false;
}

#if 0
Contract::Status Executor::ExecuteContract(ConstByteArray const &name, LaneIndex log2_num_lanes, LaneSet const &lanes, v2::Transaction const &tx)
{
  uint64_t current_fee{tx.charge_limit()};
  Status status{Status::INEXPLICABLE_FAILURE};

  // attempt to execute the first part of the contract
  // if there is no contract then exit immediately
  if (name.empty())
  {
    return Contract::Status::OK;
  }

  // attempt to parse the full contract name from from the transaction
  Identifier contract;
  if (!contract.Parse(name))
  {
    return Contract::Status::FAILED;
  }

  // create the cache and state sentinel (lock and unlock resources as well as sandbox)
  CachedStorageAdapter storage_cache{*storage_};
  StateSentinelAdapter storage_adapter{storage_cache, contract.GetParent(), log2_num_lanes, lanes};

  // lookup or create the instance of the contract as is needed
  auto chain_code = chain_code_cache_.Lookup(contract.GetParent(), *storage_);
  if (!chain_code)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Chain code lookup failure!");
    return Contract::Status::FAILED;
  }

  // attach the chain code to the current working context
  chain_code->Attach(storage_adapter);

  // Dispatch the transaction to the contract
  FETCH_LOG_DEBUG(LOGGING_NAME, "Dispatch: ", contract.name());
  auto const contract_status = chain_code->DispatchTransaction(contract.name(), tx);

  // detach the chain code from the current context
  chain_code->Detach();

  // force the flushing of the cache *** only if the contract was successful ***
  if (Contract::Status::OK == contract_status)
  {
    storage_cache.Flush();
  }

  switch (contract_status)
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
#endif

}  // namespace ledger
}  // namespace fetch
