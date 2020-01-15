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

#include "crypto/identity.hpp"
#include "ledger/upow/basic_synergetic_contract_analyser.hpp"
#include "ledger/upow/synergetic_contract.hpp"
#include "ledger/state_adapter.hpp"
#include "ledger/upow/work.hpp"
#include "ledger/chaincode/smart_contract_factory.hpp"


#include <random>


namespace fetch {
namespace ledger {

namespace {

using UInt256  = vectorise::UInt<256>;

constexpr char const *LOGGING_NAME = "BasicSynergeticContractAnalyser";
constexpr uint64_t const CHARGE_LIMIT = 1000000000;

std::random_device random_device;

}

BasicSynergeticContractAnalyser::BasicSynergeticContractAnalyser(fetch::ledger::StorageInterface &storage, crypto::Identity miner, std::size_t num_lanes)
  : storage_{storage}
  , miner_{std::move(miner)}
  , num_lanes_{num_lanes}
{
}


std::unique_ptr<SynergeticContract> BasicSynergeticContractAnalyser::GetContract(chain::Address const &contract_address)
{
  auto contract = CreateSmartContract<SynergeticContract>(contract_address, storage_);

  contract->SetChargeLimit(CHARGE_LIMIT);

  // if no contract can be loaded then simple return
  if (!contract)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to look up contract: ", contract_address.display());

    return {};
  }

  return contract;
}


BasicSynergeticContractAnalyser::Variant BasicSynergeticContractAnalyser::AnalyseContract(chain::Address const &contract_address, ProblemData const &problem_data)
{
  auto analysis_result{Variant::Object()};

  auto contract = GetContract(contract_address);

  if (!contract)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Analysis of the contract failed: ", contract_address.display());
    return {};
  }

  uint64_t charge = 0;

  //Charge for problem definition
  auto status = contract->DefineProblem(problem_data);
  if (SynergeticContract::Status::SUCCESS != status)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Analysis failed: unable to define the problem. Reason: ", ToString(status));
    return {};
  }

  auto c = contract->CalculateFee();
  analysis_result["problem"] = c;
  charge = c;

  //Charge for work execution
  auto work = std::make_shared<Work>(contract_address, miner_);
  work->UpdateNonce(UInt256(random_device()));
  WorkScore score;
  status = contract->Work(work->CreateHashedNonce(), score);

  if (SynergeticContract::Status::SUCCESS != status)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Analysis failed: unable to execute work. Reason: ", ToString(status));
    return {};
  }

  c = contract->CalculateFee();
  analysis_result["work"] = c - charge;
  charge = c;

  //complete function charge
  BitVector shard_mask{num_lanes_};
  shard_mask.SetAllOne();
  contract->Attach(storage_);

  status = contract->Complete(work->address(), shard_mask, []() -> bool {
          return false;
  });

  if (SynergeticContract::Status::VALIDATION_ERROR != status)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Analysis failed: complete contract error: ", contract_address.display(),
                   " Reason: ", ToString(status));
    return {};
  }

  analysis_result["clear"] = contract->CalculateFee() - charge;

  return analysis_result;
}

} // namespace ledger
} // namespace fetch