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
#include "core/byte_array/encoders.hpp"
#include "ledger/chaincode/contract.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/contract_context_attacher.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/chaincode/wallet_record.hpp"
#include "ledger/consensus/stake_manager.hpp"
#include "ledger/consensus/stake_update_interface.hpp"
#include "ledger/executor.hpp"
#include "ledger/fees/storage_fee.hpp"
#include "ledger/state_sentinel_adapter.hpp"
#include "ledger/storage_unit/cached_storage_adapter.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/utils/timer.hpp"

#include <algorithm>
#include <exception>
#include <memory>
#include <utility>

static constexpr char const *LOGGING_NAME    = "Executor";
static constexpr uint64_t    TRANSFER_CHARGE = 1;

using fetch::telemetry::Histogram;
using fetch::telemetry::Registry;

namespace fetch {
namespace ledger {

/**
 * Construct a Executor given a storage unit
 *
 * @param storage The storage unit to be used
 */
Executor::Executor(StorageUnitPtr storage)
  : storage_{std::move(storage)}
  , tx_validator_{*storage_, token_contract_}
  , fee_manager_{token_contract_, "ledger_executor_deduct_fees_duration"}
  , overall_duration_{Registry::Instance().LookupMeasurement<Histogram>(
        "ledger_executor_overall_duration")}
  , tx_retrieve_duration_{Registry::Instance().LookupMeasurement<Histogram>(
        "ledger_executor_tx_retrieve_duration")}
  , validation_checks_duration_{Registry::Instance().LookupMeasurement<Histogram>(
        "ledger_executor_validation_checks_duration")}
  , contract_execution_duration_{Registry::Instance().LookupMeasurement<Histogram>(
        "ledger_executor_contract_execution_duration")}
  , transfers_duration_{Registry::Instance().LookupMeasurement<Histogram>(
        "ledger_executor_transfers_duration")}
  , settle_fees_duration_{
        Registry::Instance().LookupMeasurement<Histogram>("ledger_executor_settle_fees_duration")}
{}

/**
 * Executes a given transaction across a series of lanes
 *
 * @param digest The transaction digest to be executed
 * @param block The current block index
 * @param slice The current slice index
 * @param shards The bit vector outlining the shards in use by this transaction
 * @return The status code for the operation
 */
Executor::Result Executor::Execute(Digest const &digest, BlockIndex block, SliceIndex slice,
                                   BitVector const &shards)
{
  telemetry::FunctionTimer const timer{*overall_duration_};

  FETCH_LOG_DEBUG(LOGGING_NAME, "Executing tx ", byte_array::ToBase64(digest));

  Result result{Status::INEXPLICABLE_FAILURE};

  // cache the state for the current transaction
  block_          = block;
  slice_          = slice;
  allowed_shards_ = shards;
  log2_num_lanes_ = shards.log2_size();

  // attempt to retrieve the transaction from the storage
  if (!RetrieveTransaction(digest))
  {
    // signal that the contract failed to be executed
    result.status = Status::TX_LOOKUP_FAILURE;
  }
  else
  {
    // update the charge related data provided by Tx sender
    result.charge_rate  = current_tx_->charge_rate();
    result.charge_limit = current_tx_->charge_limit();

    // create the storage cache
    storage_cache_ = std::make_shared<CachedStorageAdapter>(*storage_);

    // follow the three step process for executing a transaction
    //
    // 0. Validation checks (does the originator have correct funds)
    // 1. Execute the containing transaction
    // 2. Execute any token transfers
    // 3. Process the fees
    //
    bool const success =
        ValidationChecks(result) && ExecuteTransactionContract(result) && ProcessTransfers(result);

    if (!success)
    {
      // in addition to avoid indeterminate data being partially flushed. In the case of the when
      // the transaction execution fails then we also clear all the cached data.
      storage_cache_->Clear();
    }

    FeeManager::TransactionDetails tx_details{*current_tx_, allowed_shards_};

    // deduct the fees from the originator
    fee_manager_.Execute(tx_details, result, block_, *storage_cache_);

    // flush the storage so that all changes are now persistent
    storage_cache_->Flush();
  }

  return result;
}

void Executor::SettleFees(chain::Address const &miner, BlockIndex block, TokenAmount amount,
                          uint32_t log2_num_lanes, StakeUpdateEvents const &stake_updates)
{
  telemetry::FunctionTimer const timer{*settle_fees_duration_};

  FETCH_LOG_TRACE(LOGGING_NAME, "Settling fees");

  fee_manager_.SettleFees(miner, amount, current_tx_->contract_address(), log2_num_lanes, block_,
                          *storage_);

  FETCH_LOG_TRACE(LOGGING_NAME, "Aggregating stake updates...");

  if (!stake_updates.empty())
  {
    StakeManager stake_manager{};
    if (stake_manager.Load(*storage_))
    {
      auto &update_queue = stake_manager.update_queue();

      for (auto const &update : stake_updates)
      {
        update_queue.AddStakeUpdate(update.block_index, update.from, update.amount);
      }

      // provide aggregation and clean up of the resources
      stake_manager.UpdateCurrentBlock(block);

      if (!stake_manager.Save(*storage_))
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Unable to save stake manager updates");
      }
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to load stake manager updates");
    }
  }

  FETCH_LOG_TRACE(LOGGING_NAME, "Aggregating stake updates...complete");
}

bool Executor::RetrieveTransaction(Digest const &digest)
{
  telemetry::FunctionTimer const timer{*tx_retrieve_duration_};

  bool success{false};

  try
  {
    // create a new transaction
    current_tx_ = std::make_unique<chain::Transaction>();

    // load the transaction from the store
    success = storage_->GetTransaction(digest, *current_tx_);
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Exception caught when retrieving tx from store: ", ex.what());
  }

  return success;
}

bool Executor::ValidationChecks(Result &result)
{
  telemetry::FunctionTimer const timer{*validation_checks_duration_};

  // validate this transaction at this time point
  auto const status = tx_validator_(*current_tx_, block_);

  if (status != Status::SUCCESS)
  {
    result.status = status;
    return false;
  }

  // All checks passed
  result.status = Status::SUCCESS;
  return true;
}

bool Executor::ExecuteTransactionContract(Result &result)
{
  using ContractMode = chain::Transaction::ContractMode;
  telemetry::FunctionTimer const timer{*contract_execution_duration_};

  bool success{false};

  try
  {
    ConstByteArray contract_id{};
    switch (current_tx_->contract_mode())
    {
    case ContractMode::PRESENT:
      contract_id = current_tx_->contract_address().display();
      break;
    case ContractMode::CHAIN_CODE:
      contract_id = current_tx_->chain_code();
      break;
    case ContractMode::NOT_PRESENT:
      break;
    case ContractMode::SYNERGETIC:
      // synergetic contracts are not supported through normal pipeline
      break;
    }

    // when there is no contract signalled in the transaction the identifier will be empty. This is
    // a normal use case
    if (contract_id.empty())
    {
      result.status = Status::SUCCESS;
      return true;
    }

    // create the cache and state sentinel (lock and unlock resources as well as sandbox)
    StateSentinelAdapter storage_adapter{*storage_cache_, contract_id, allowed_shards_};

    // look up or create the instance of the contract as is needed
    bool const is_token_contract = (contract_id == "fetch.token");

    Contract *contract = is_token_contract ? &token_contract_
                                           : chain_code_cache_.Lookup(contract_id, *storage_).get();
    if (!static_cast<bool>(contract))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Contract lookup failure: ", contract_id);
      result.status = Status::CONTRACT_LOOKUP_FAILURE;
      return false;
    }

    // Dispatch the transaction to the contract
    FETCH_LOG_DEBUG(LOGGING_NAME, "Dispatch: ", current_tx_->action());

    Contract::Result contract_status;
    {
      ContractContext context{&token_contract_, current_tx_->contract_address(), storage_.get(),
                              &storage_adapter, block_};
      ContractContextAttacher raii(*contract, context);
      contract_status = contract->DispatchTransaction(*current_tx_);
    }

    // map the contract execution status
    result.status = Status::CONTRACT_EXECUTION_FAILURE;
    switch (contract_status.status)
    {
    case Contract::Status::OK:
      success       = true;
      result.status = Status::SUCCESS;
      break;
    case Contract::Status::FAILED:
      FETCH_LOG_WARN(LOGGING_NAME, "Transaction execution failed!");
      break;
    case Contract::Status::NOT_FOUND:
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to look up transaction handler");
      result.status = Status::ACTION_LOOKUP_FAILURE;
      break;
    }

    result.return_value = contract_status.return_value;

    FETCH_LOG_DEBUG(LOGGING_NAME, "Executing tx ", byte_array::ToBase64(current_tx_->digest()),
                    " (success)");

    // attempt to generate a fee for this transaction
    if (success)
    {
      // simple linear scale fee

      StorageFee storage_fee{storage_adapter};

      FeeManager::TransactionDetails tx_details{*current_tx_, allowed_shards_};

      success =
          fee_manager_.CalculateChargeAndValidate(tx_details, {contract, &storage_fee}, result);

      if (success)
      {
        token_contract_.ExtractStakeUpdates(result.stake_updates);
      }

      token_contract_.ClearStakeUpdates();
    }
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Exception during execution of tx 0x",
                   current_tx_->digest().ToHex(), " : ", ex.what());

    result.status = Status::CONTRACT_EXECUTION_FAILURE;
  }

  return success;
}

bool Executor::ProcessTransfers(Result &result)
{
  telemetry::FunctionTimer const timer{*transfers_duration_};

  bool success{true};

  // ensure that we have transfers to make
  if (!current_tx_->transfers().empty())
  {
    // attach the token contract to the storage engine
    StateSentinelAdapter storage_adapter{*storage_cache_, "fetch.token", allowed_shards_};

    ContractContext         context{&token_contract_, current_tx_->contract_address(), nullptr,
                            &storage_adapter, block_};
    ContractContextAttacher raii(token_contract_, context);

    // only process transfers if the previous steps have been successful
    if (Status::SUCCESS == result.status)
    {
      // make all the transfers that are necessary
      for (auto const &transfer : current_tx_->transfers())
      {
        // make the individual transfers
        if (!token_contract_.TransferTokens(*current_tx_, transfer.to, transfer.amount))
        {
          result.status = Status::TRANSFER_FAILURE;
          success       = false;
          break;
        }

        // the cost in charge units to make a transfer
        result.charge += TRANSFER_CHARGE;
      }
    }
  }

  return success;
}

}  // namespace ledger
}  // namespace fetch
