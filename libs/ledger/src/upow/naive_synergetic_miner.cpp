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
#include "ledger/upow/synergetic_job.hpp"
#include "ledger/upow/work.hpp"
#include "logging/logging.hpp"
#include "vm_modules/math/bignumber.hpp"

#include <random>
#include <unordered_set>

namespace fetch {
namespace ledger {
namespace {

constexpr char const *LOGGING_NAME = "NaiveSynMiner";

constexpr uint64_t const CHARGE_LIMIT               = 10000000000;
constexpr uint64_t const ANALYSER_CHARGE_LIMIT      = 100000;
constexpr uint64_t const ANALYSER_CHARGE_LIMIT_INC  = 10;
constexpr uint8_t const  ANALYSER_CHARGE_LIMIT_LOOP = 3;

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

NaiveSynergeticMiner::NaiveSynergeticMiner(DAGPtr dag, StorageInterface &storage, ProverPtr prover,
                                           ConstByteArray const &script,
                                           ContractAnalyserPtr   contract_analyser)
  : dag_{std::move(dag)}
  , storage_{storage}
  , prover_{std::move(prover)}
  , state_machine_{std::make_shared<core::StateMachine<State>>("NaiveSynMiner", State::INITIAL)}
  , job_script_{script}
  , contract_analyser_{std::move(contract_analyser)}
  , miner_address_{prover_->identity()}
{
  state_machine_->RegisterHandler(State::INITIAL, this, &NaiveSynergeticMiner::OnInitial);
  state_machine_->RegisterHandler(State::MINE, this, &NaiveSynergeticMiner::OnMine);
  job_script_.set_balance(GetBalance());
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
  uint64_t previous_epoch;
  auto     dag_nodes = dag_->GetLatest(true, previous_epoch);

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

  // Anlyise the problem space
  std::vector<SynergeticContractAnalyserInterface::SynergeticJobPtr> job_descriptions{};
  std::unordered_map<std::size_t, chain::Address>                    address_lookup{};
  std::size_t                                                        id = 0;
  for (auto const &problem : problem_spaces)
  {
    SynergeticContractAnalyserInterface::SynergeticJobPtr res{};
    auto                                                  charge_limit = ANALYSER_CHARGE_LIMIT;
    for (uint8_t i = 0; i < ANALYSER_CHARGE_LIMIT_LOOP; ++i)
    {
      res = contract_analyser_->AnalyseContract(problem.first, problem.second, charge_limit);
      if (res)
      {
        break;
      }
      charge_limit *= ANALYSER_CHARGE_LIMIT_INC;
    }
    if (!res)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Failed to analyse contract ", problem.first.display(), "!");
      continue;
    }
    res->set_id(id);
    res->set_contract_address(problem.first);
    res->set_epoch(previous_epoch);
    address_lookup[id] = problem.first;
    ++id;
    FETCH_LOG_INFO(LOGGING_NAME, "Contract ", res->id(),
                   " analysis: problem=", res->problem_charge(), ", work=", res->work_charge(),
                   ", clear=", res->clear_charge());
    job_descriptions.push_back(std::move(res));
  }

  std::vector<uint64_t> selected_jobs{};
  auto status = job_script_.GenerateJobList(job_descriptions, selected_jobs, GetBalance());

  if (status != SynergeticMinerScript::Status::SUCCESS)
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Failed to generate job list using synergetic miner script! Falling back to "
                   "naive version...");
    selected_jobs.clear();
    for (std::size_t i = 0; i < job_descriptions.size(); ++i)
    {
      selected_jobs.push_back(i);
    }
  }

  uint64_t expected_charge{0};
  for (auto const &job : selected_jobs)
  {
    auto it = address_lookup.find(job);
    if (it == address_lookup.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Job selection algorithm returned invalid ID (", job, ")!");
      continue;
    }
    FETCH_LOG_WARN(LOGGING_NAME, "Job with ", job, " id is selected!");
    auto const &problem = problem_spaces[it->second];

    uint64_t job_expected_charge;
    // attempt to mine a solution to this problem
    auto const solution = MineSolution(it->second, problem, job_expected_charge);

    // check to see if a solution was generated
    if (solution)
    {
      dag_->AddWork(*solution);
      expected_charge += job_expected_charge;

      FETCH_LOG_DEBUG(LOGGING_NAME, "Mined and added work! Epoch number: ", dag_->CurrentEpoch());
    }
  }

  job_script_.set_back_expected_charge(static_cast<int64_t>(expected_charge));
}

void NaiveSynergeticMiner::EnableMining(bool enable)
{
  is_mining_ = enable;
}

WorkPtr NaiveSynergeticMiner::MineSolution(chain::Address const &contract_address,
                                           ProblemData const &   problem_data,
                                           uint64_t &            expected_charge)
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
      expected_charge = contract->CalculateFee();
      // TODO(AB): scaling? charge approx search_length_*one
      if (expected_charge >= balance)
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

uint64_t NaiveSynergeticMiner::GetBalance()
{
  StateAdapter            storage_adapter{storage_, "fetch.token"};
  ContractContext         context{&token_contract_, miner_address_, nullptr, &storage_adapter, 0};
  ContractContextAttacher raii(token_contract_, context);
  return token_contract_.GetBalance(miner_address_);
}

}  // namespace ledger
}  // namespace fetch
