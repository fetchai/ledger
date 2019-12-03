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

#include "core/bitvector.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/smart_contract_factory.hpp"
#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/state_sentinel_adapter.hpp"
#include "ledger/upow/synergetic_contract.hpp"
#include "ledger/upow/synergetic_executor.hpp"
#include "logging/logging.hpp"

#include <cstddef>
#include <cstdint>

namespace fetch {
namespace ledger {

constexpr char const *LOGGING_NAME = "SynergeticExecutor";

SynergeticExecutor::SynergeticExecutor(StorageInterface &storage)
  : storage_{storage}
{}

void SynergeticExecutor::Verify(WorkQueue &solutions, ProblemData const &problem_data,
                                std::size_t num_lanes)
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
      auto const &address = solution->address().display();

      // create the contract
      contract = CreateSmartContract<SynergeticContract>(address, storage_);

      if (!contract)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to create synergetic contract: ", address);
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
    WorkScore calculated_score{0};
    auto      status = contract->Work(solution->CreateHashedNonce(), calculated_score);

    if (SynergeticContract::Status::SUCCESS == status && calculated_score == solution->score())
    {
      // TODO(issue 1213): State sharding needs to be added here
      BitVector shard_mask{num_lanes};
      shard_mask.SetAllOne();

      StateSentinelAdapter storage_adapter{storage_, solution->address().display(), shard_mask};

      // complete the work and resolve the work queue
      contract->Attach(storage_);
      ContractContext ctx(&token_contract_, solution->address(), &storage_adapter, 0);
      contract->UpdateContractContext(ctx);
      status = contract->Complete(solution->address(), shard_mask);
      contract->Detach();

      if (SynergeticContract::Status::SUCCESS != status)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to complete contract: 0x", contract->digest().ToHex(),
                       " Reason: ", ToString(status));
        return;
      }
      break;
    }

    FETCH_LOG_WARN(LOGGING_NAME, "Best solution is not valid, trying next solution");
  }
}

}  // namespace ledger
}  // namespace fetch
