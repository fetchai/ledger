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

#include "chain/transaction_builder.hpp"
#include "core/bitvector.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/smart_contract_factory.hpp"
#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/execution_result.hpp"
#include "ledger/state_sentinel_adapter.hpp"
#include "ledger/upow/synergetic_contract.hpp"
#include "ledger/upow/synergetic_executor.hpp"
#include "logging/logging.hpp"
#include "meta/log2.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/utils/timer.hpp"

#include <cstddef>
#include <cstdint>

namespace fetch {
namespace ledger {

namespace {

constexpr char const *LOGGING_NAME = "SynergeticExecutor";

constexpr TokenAmount const CHARGE_RATE  = 1;
constexpr TokenAmount const CHARGE_LIMIT = 10000000000;

using fetch::telemetry::Histogram;
using fetch::telemetry::Registry;
using fetch::meta::Log2;

}  // namespace

SynergeticExecutor::SynergeticExecutor(StorageInterface &storage)
  : storage_{storage}
  , fee_manager_{token_contract_, "ledger_synergetic_executor_deduct_fees_duration"}
  , work_duration_{Registry::Instance().LookupMeasurement<Histogram>(
        "ledger_synergetic_executor_work_duration")}
  , complete_duration_{Registry::Instance().LookupMeasurement<Histogram>(
        "ledger_synergetic_executor_complete_duration")}
{}

void SynergeticExecutor::Verify(WorkQueue &solutions, ProblemData const &problem_data,
                                std::size_t num_lanes, chain::Address const &miner)
{
  std::unique_ptr<SynergeticContract> contract;

  // iterate through each of the solutions
  while (!solutions.empty())
  {
    // get and extract the top element
    auto const solution = solutions.top();
    solutions.pop();

    // in the case of the first iteration we need to create the contract and define the problem
    if (!contract)
    {
      auto const &address = solution->address();

      // create the contract
      contract = CreateSmartContract<SynergeticContract>(address, storage_);

      if (!contract)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to create synergetic contract: ", address.display());
        return;
      }

      // define the problem
      auto const status = contract->DefineProblem(problem_data);
      if (SynergeticContract::Status::SUCCESS != status)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Unable to define synergetic problem: ", ToString(status));

        return;
      }
    }

    // validate the work that has been done
    WorkScore                  calculated_score{0};
    SynergeticContract::Status status;
    {
      telemetry::FunctionTimer const timer{*work_duration_};
      status = contract->Work(solution->CreateHashedNonce(), calculated_score);
    }
    // TODO(LDGR-621): fee for invalid solution?
    if (SynergeticContract::Status::SUCCESS == status && calculated_score == solution->score())
    {
      // TODO(issue 1213): State sharding needs to be added here
      BitVector shard_mask{num_lanes};
      shard_mask.SetAllOne();

      StateSentinelAdapter storage_adapter{storage_, solution->address().display(), shard_mask};

      // complete the work and resolve the work queue
      contract->Attach(storage_);
      ContractContext ctx(&token_contract_, solution->address(), nullptr, &storage_adapter, 0);
      contract->UpdateContractContext(ctx);

      // TODO(LDGR-622): charge limit
      FeeManager::TransactionDetails tx_details{solution->address(), solution->address(),
                                                shard_mask,          solution->address().display(),
                                                CHARGE_RATE,         CHARGE_LIMIT};

      ContractExecutionResult result;

      {
        telemetry::FunctionTimer const timer{*complete_duration_};
        status = contract->Complete(
            solution->address(), shard_mask, [this, &contract, &tx_details, &result]() -> bool {
              return fee_manager_.CalculateChargeAndValidate(tx_details, {contract.get()}, result);
            });
      }

      if (SynergeticContract::Status::SUCCESS != status)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to complete contract: 0x", contract->digest().ToHex(),
                       " Reason: ", ToString(status));
        return;
      }
      FETCH_LOG_DEBUG(LOGGING_NAME, "Calculated fee: ", result.charge);
      fee_manager_.Execute(tx_details, result, solution->block_index(), storage_);

      fee_manager_.SettleFees(miner, result.fee, tx_details.contract_address,
                              Log2(static_cast<uint32_t>(num_lanes)), solution->block_index(),
                              storage_);

      contract->Detach();

      break;
    }

    FETCH_LOG_WARN(LOGGING_NAME, "Best solution is not valid, trying next solution");
  }
}

}  // namespace ledger
}  // namespace fetch
