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

#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/upow/synergetic_executor.hpp"

#include <limits>

namespace fetch {
namespace ledger {
namespace {

constexpr char const *LOGGING_NAME = "SynExec";

} // namespace

SynergeticExecutor::SynergeticExecutor(StorageInterface &storage)
  : factory_{storage}
{
}

void SynergeticExecutor::Verify(WorkQueue &solutions)
{
  SynergeticContractPtr contract{};

  // iterate through each of the solutions
  while (!solutions.empty())
  {
    // get and extract the top element
    auto const solution = solutions.top();
    solutions.pop();

    // in the case of the first iteration we need to create the contract and define the problem
    if (!contract)
    {
      auto const &contract_address = solution->contract_digest();

      // create the contract
      contract = factory_.Create(contract_address);
      if (!contract)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to create contract: 0x", contract_address.address().ToHex());
        return;
      }

      // define the problem
      auto const status = contract->DefineProblem();
      if (SynergeticContract::Status::SUCCESS != status)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Unable to define synergetic problem: ", ToString(status));
        return;
      }
    }

    auto const verification_nonce = solution->CreateHashedNonce();

    FETCH_LOG_INFO(LOGGING_NAME, "Verified Nonce: ", verification_nonce.ToHex());

    // validate the work that has been done
    WorkScore calculated_score{0};
    auto const status = contract->Work(verification_nonce, calculated_score);

    FETCH_LOG_INFO(LOGGING_NAME, "Verified Score: ", calculated_score, " Expected Score: ", solution->score());

    if ((SynergeticContract::Status::SUCCESS == status) && (calculated_score == solution->score()))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "$$$ I would normally apply those state changes now $$$");

      // TODO(EJF): Apply statue changes
      break;
    }

    FETCH_LOG_WARN(LOGGING_NAME, "Best solution is not valid, trying next solution");
  }
}

} // namespace ledger
} // namespace fetch
