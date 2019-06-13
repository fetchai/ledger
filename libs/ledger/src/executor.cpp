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
#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/contract.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/storage_unit/cached_storage_adapter.hpp"
#include "metrics/metrics.hpp"

#include "ledger/state_sentinel_adapter.hpp"

#include <algorithm>
#include <chrono>
#include <random>

static constexpr char const *LOGGING_NAME    = "Executor";
static constexpr uint64_t    TRANSFER_CHARGE = 1;

using fetch::byte_array::ConstByteArray;
using fetch::storage::ResourceAddress;

#ifdef FETCH_ENABLE_METRICS
using fetch::metrics::Metrics;
#endif  // FETCH_ENABLE_METRICS

namespace fetch {
namespace ledger {
namespace {

bool GenerateContractName(Transaction const &tx, Identifier &identifier)
{
  // Step 1 - Translate the tx into a common name
  using ContractMode = Transaction::ContractMode;

  ConstByteArray contract_name{};
  switch (tx.contract_mode())
  {
  case ContractMode::NOT_PRESENT:
    break;
  case ContractMode::PRESENT:
    contract_name = tx.contract_digest().address().ToHex() + "." + tx.contract_address().display() +
                    "." + tx.action();
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

bool IsCreateWealth(Transaction const &tx)
{
  return (tx.contract_mode() == Transaction::ContractMode::CHAIN_CODE) &&
         (tx.chain_code() == "fetch.token") && (tx.action() == "wealth");
}

}  // namespace

/**
 * Construct a Executor given a storage unit
 *
 * @param storage The storage unit to be used
 */
Executor::Executor(StorageUnitPtr storage)
  : storage_{std::move(storage)}
  , token_contract_{std::make_shared<TokenContract>()}
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
  FETCH_LOG_DEBUG(LOGGING_NAME, "Executing tx ", byte_array::ToBase64(digest));

  Result result{Status::INEXPLICABLE_FAILURE, 0, 0, 0};

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
    // update the charge rate
    result.charge_rate = current_tx_->charge();

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

    // deduct the fees from the originator
    DeductFees(result);

    // flush the storage so that all changes are now persistent
    storage_cache_->Flush();
  }

  // clean up any used resources
  Cleanup();

  return result;
}

void Executor::SettleFees(Address const &miner, TokenAmount amount, uint32_t log2_num_lanes)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Settling fees");

  // only if there are fees to settle then update the state database
  if (amount > 0)
  {
    // compute the resource address
    ResourceAddress resource_address{"fetch.token.state." + miner.display()};

    // create the complete shard mask
    BitVector shard{1u << log2_num_lanes};
    shard.set(resource_address.lane(log2_num_lanes), 1);

    // attach the token contract to the storage engine
    StateSentinelAdapter storage_adapter{*storage_, Identifier{"fetch.token"}, shard};
    token_contract_->Attach(storage_adapter);
    token_contract_->AddTokens(miner, amount);
    token_contract_->Detach();
  }
}

bool Executor::RetrieveTransaction(Digest const &digest)
{
  bool success{false};

  try
  {
    // create a new transaction
    current_tx_ = std::make_unique<Transaction>();

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
  // SHORT TERM EXEMPTION - While no state file exists (and the wealth endpoint is still present)
  // this and only this contract is exempt from the pre-validation checks
  if (IsCreateWealth(*current_tx_))
  {
    result.status = Status::SUCCESS;
    return true;
  }

  // CHECK: Determine if the transaction is valid for the given block
  auto const tx_validity = current_tx_->GetValidity(block_);
  if (Transaction::Validity::VALID != tx_validity)
  {
    result.status = Status::TX_NOT_VALID_FOR_BLOCK;
    return false;
  }

  // attach the token contract to the storage engine
  StateAdapter storage_adapter{*storage_cache_, Identifier{"fetch.token"}};
  token_contract_->Attach(storage_adapter);

  // lookup the balance from the transaction originator
  uint64_t const balance = token_contract_->GetBalance(current_tx_->from());

  // detach the token transfers
  token_contract_->Detach();

  // CHECK: Ensure that the originator has funds available to make both all the transfers in the
  //        contract as well as the maximum fees
  uint64_t const max_tokens_required =
      current_tx_->GetTotalTransferAmount() + current_tx_->charge_limit();
  if (balance < max_tokens_required)
  {
    result.status = Status::INSUFFICIENT_AVAILABLE_FUNDS;
    return false;
  }

  // All checks passed
  result.status = Status::SUCCESS;
  return true;
}

bool Executor::ExecuteTransactionContract(Result &result)
{
  bool success{false};

  try
  {
    Identifier contract_id{};

    // generate the contract name (identifier)
    if (!GenerateContractName(*current_tx_, contract_id))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to generate the contract name");
      result.status = Status::CONTRACT_NAME_PARSE_FAILURE;
      return false;
    }

    // when there is no contract signalled in the transaction the identifier will be empty. This is
    // a normal use case
    if (contract_id.empty())
    {
      result.status = Status::SUCCESS;
      return true;
    }

    // create the cache and state sentinel (lock and unlock resources as well as sandbox)
    StateSentinelAdapter storage_adapter{*storage_cache_, contract_id.GetParent(), allowed_shards_};

    // lookup or create the instance of the contract as is needed
    auto contract = chain_code_cache_.Lookup(contract_id.GetParent(), *storage_);
    if (!contract)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Contract lookup failure: ", contract_id.full_name());
      result.status = Status::CONTRACT_LOOKUP_FAILURE;
    }

    // attach the chain code to the current working context
    contract->Attach(storage_adapter);

    // Dispatch the transaction to the contract
    FETCH_LOG_DEBUG(LOGGING_NAME, "Dispatch: ", contract_id.name());
    auto const contract_status =
        contract->DispatchTransaction(contract_id.name(), *current_tx_, block_);

    // detach the chain code from the current context
    contract->Detach();

    // map the contract execution status
    result.status = Status::CHAIN_CODE_EXEC_FAILURE;
    switch (contract_status)
    {
    case Contract::Status::OK:
      success       = true;
      result.status = Status::SUCCESS;
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

    FETCH_LOG_DEBUG(LOGGING_NAME, "Executing tx ", byte_array::ToBase64(current_tx_->digest()),
                    " (success)");

    FETCH_METRIC_TX_EXEC_STARTED_EX(hash, started);
    FETCH_METRIC_TX_EXEC_COMPLETE_EX(hash, completed);

    // attempt to generate a fee for this transaction
    if (success)
    {
      // simple linear scale fee
      uint64_t const compute_charge = contract->CalculateFee();
      uint64_t const storage_charge = (storage_adapter.num_bytes_written() * 2u);
      uint64_t const base_charge    = compute_charge + storage_charge;
      uint64_t const scaled_charge =
          std::max<uint64_t>(allowed_shards_.PopCount(), 1) * base_charge;

      FETCH_LOG_INFO(LOGGING_NAME, "Calculated charge for 0x", current_tx_->digest().ToHex(), ": ",
                     scaled_charge, " (base: ", base_charge, " storage: ", storage_charge,
                     " compute: ", compute_charge, " shards: ", allowed_shards_.PopCount(), ")");

      // create wealth transactions are always free
      if (!IsCreateWealth(*current_tx_))
      {
        result.charge += scaled_charge;
      }

      // determine if the chain code ran out of charge
      if (result.charge > current_tx_->charge_limit())
      {
        result.status = Status::INSUFFICIENT_CHARGE;
        success       = false;
      }
    }
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Exception during execution of tx 0x",
                   current_tx_->digest().ToHex(), " : ", ex.what());

    result.status = Status::CHAIN_CODE_EXEC_FAILURE;
  }

  return success;
}

bool Executor::ProcessTransfers(Result &result)
{
  bool success{true};

  // attach the token contract to the storage engine
  StateSentinelAdapter storage_adapter{*storage_cache_, Identifier{"fetch.token"}, allowed_shards_};
  token_contract_->Attach(storage_adapter);

  // only process transfers if the previous steps have been successful
  if (Status::SUCCESS == result.status)
  {
    // make all the transfers that are necessary
    for (auto const &transfer : current_tx_->transfers())
    {
      // make the individual transfers
      if (!token_contract_->TransferTokens(*current_tx_, transfer.to, transfer.amount))
      {
        result.status = Status::TRANSFER_FAILURE;
        success       = false;
        break;
      }
      else
      {
        // the cost in charge units to make a transfer
        result.charge += TRANSFER_CHARGE;
      }
    }
  }

  // detach the contract
  token_contract_->Detach();

  return success;
}

void Executor::DeductFees(Result &result)
{
  // attach the token contract to the storage engine
  StateSentinelAdapter storage_adapter{*storage_cache_, Identifier{"fetch.token"}, allowed_shards_};
  token_contract_->Attach(storage_adapter);

  auto const &from = current_tx_->from();

  // lookup the account balance
  uint64_t const balance = token_contract_->GetBalance(from);

  // calculate the fee to deduct
  TokenAmount tx_fee = result.charge * current_tx_->charge();
  if (Status::SUCCESS != result.status)
  {
    tx_fee = current_tx_->charge_limit() * current_tx_->charge();
  }

  // on failed transactions
  result.fee = std::min(balance, tx_fee);

  // deduct the fee from the originator
  token_contract_->SubtractTokens(from, result.fee);

  token_contract_->Detach();
}

bool Executor::Cleanup()
{
  return false;
}

}  // namespace ledger
}  // namespace fetch
