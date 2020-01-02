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

#include "core/digest.hpp"
#include "core/serializers/main_serializer.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/contract_context_attacher.hpp"
#include "ledger/chaincode/smart_contract_factory.hpp"
#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/state_adapter.hpp"
#include "ledger/upow/naive_synergetic_miner.hpp"
#include "ledger/upow/synergetic_base_types.hpp"
#include "ledger/upow/work.hpp"
#include "logging/logging.hpp"
#include "vm_modules/math/bignumber.hpp"

#include <random>
#include <unordered_set>

namespace fetch {
namespace ledger {
namespace {

constexpr char const *LOGGING_NAME = "NaiveSynMiner";

constexpr uint64_t const CHARGE_LIMIT = 10000000000;

using UInt256  = vectorise::UInt<256>;
using DagNodes = NaiveSynergeticMiner::DagNodes;

void ExecuteWork(SynergeticContract &contract, WorkPtr const &work)
{
  WorkScore score{0};

  // execute the work
  auto const nonce_work = work->CreateHashedNonce();

  auto const status = contract.Work(nonce_work, score);

  if (SynergeticContract::Status::SUCCESS != status)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to execute work. Reason: ", ToString(status));

    // set the score to highest possible value
    score = std::numeric_limits<WorkScore>::max();
  }

  // update the score for the piece of work
  work->UpdateScore(score);
  FETCH_LOG_DEBUG(LOGGING_NAME, "Execute Nonce: 0x", std::string(nonce_work), " score: ", score);
}

}  // namespace

NaiveSynergeticMiner::NaiveSynergeticMiner(DAGPtr dag, StorageInterface &storage, ProverPtr prover)
  : dag_{std::move(dag)}
  , storage_{storage}
  , prover_{std::move(prover)}
  , state_machine_{std::make_shared<core::StateMachine<State>>("NaiveSynMiner", State::INITIAL)}
{
  state_machine_->RegisterHandler(State::INITIAL, this, &NaiveSynergeticMiner::OnInitial);
  state_machine_->RegisterHandler(State::MINE, this, &NaiveSynergeticMiner::OnMine);
}

core::WeakRunnable NaiveSynergeticMiner::GetWeakRunnable()
{
  return state_machine_;
}

NaiveSynergeticMiner::State NaiveSynergeticMiner::OnInitial()
{
  state_machine_->Delay(std::chrono::milliseconds{200});

  return State::MINE;
}

NaiveSynergeticMiner::State NaiveSynergeticMiner::OnMine()
{
  state_machine_->Delay(std::chrono::milliseconds{200});

  if (is_mining_)
  {
    this->Mine();
  }

  return State::INITIAL;
}

void NaiveSynergeticMiner::Mine()
{
  using ProblemSpaces = std::unordered_map<chain::Address, ProblemData>;

  // iterate through the latest DAG nodes and build a complete set of addresses to mine solutions
  // for
  // TODO(HUT): would be nicer to specify here what we want by type
  auto dag_nodes = dag_->GetLatest(true);

  // loop through the data that is available for the previous epoch
  ProblemSpaces problem_spaces{};
  for (auto const &node : dag_nodes)
  {
    if (DAGNode::DATA == node.type)
    {
      // look up the problem data
      auto &problem_data = problem_spaces[node.contract_address];

      problem_data.emplace_back(node.contents);
    }
  }

  // no mining can be performed when no work is available
  if (problem_spaces.empty())
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "No data to be mined");
    return;
  }

#ifdef FETCH_LOG_DEBUG_ENABLED
  {
    std::ostringstream oss;
    for (auto const &element : problem_spaces)
    {
      oss << "\n -> 0x" << element.first.contract_address.display();
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "Available synergetic contracts to be mined", oss.str());
  }
#endif  // FETCH_LOG_DEBUG_ENABLED

  // for each of the contract addresses available mine a solution
  for (auto const &problem : problem_spaces)
  {
    // attempt to mine a solution to this problem
    auto const solution = MineSolution(problem.first, problem.second);

    // check to see if a solution was generated
    if (solution)
    {
      dag_->AddWork(*solution);

      FETCH_LOG_DEBUG(LOGGING_NAME, "Mined and added work! Epoch number: ", dag_->CurrentEpoch());
    }
  }
}

void NaiveSynergeticMiner::EnableMining(bool enable)
{
  is_mining_ = enable;
}

WorkPtr NaiveSynergeticMiner::MineSolution(chain::Address const &contract_address,
                                           ProblemData const &   problem_data)
{
  StateAdapter storage_adapter{storage_, "fetch.token"};

  ContractContext         context{&token_contract_, contract_address, nullptr, &storage_adapter, 0};
  ContractContextAttacher raii(token_contract_, context);

  uint64_t const balance = token_contract_.GetBalance(contract_address);

  if (balance == 0)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Not handling contract: ", contract_address.display(),
                   " balance is 0");
    return {};
  }

  auto contract = CreateSmartContract<SynergeticContract>(contract_address, storage_);

  contract->SetChargeLimit(CHARGE_LIMIT / search_length_);

  // if no contract can be loaded then simple return
  if (!contract)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to look up contract: ", contract_address.display());

    return {};
  }

  // build up a work instance
  auto work = std::make_shared<Work>(contract_address, prover_->identity());

  // Preparing to mine
  auto status = contract->DefineProblem(problem_data);
  if (SynergeticContract::Status::SUCCESS != status)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to define the problem. Reason: ", ToString(status));

    return {};
  }

  // update the initial nonce
  std::random_device rd;
  UInt256            nonce{rd()};

  // generate a series of solutions for each of the problems
  WorkPtr best_work{};
  for (std::size_t i = 0; i < search_length_; ++i)
  {
    // update the nonce
    work->UpdateNonce(nonce);
    ++nonce;

    // execute the work
    ExecuteWork(*contract, work);

    // update the cached work if this one is better than previous solutions
    if (!(best_work && best_work->score() >= work->score()))
    {
      best_work = std::make_shared<Work>(*work);
    }

    if (i == 0)
    {
      auto const fee = contract->CalculateFee();
      // TODO(AB): scaling? charge approx search_length_*one
      if (fee >= balance)
      {
        // not enough balance, stop using contract
        return {};
      }
    }
  }

  contract->Detach();

  // Returning the best work from this round
  return best_work;
}

}  // namespace ledger
}  // namespace fetch
