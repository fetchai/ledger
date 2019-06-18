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

#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "ledger/chain/digest.hpp"
#include "ledger/upow/naive_synergetic_miner.hpp"
#include "ledger/upow/work.hpp"

#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/upow/synergetic_base_types.hpp"

#include "vm_modules/math/bignumber.hpp"

#include <random>
#include <unordered_set>

namespace fetch {
namespace ledger {
namespace {

constexpr char const *LOGGING_NAME = "NaiveSynMiner";

using math::BigUnsigned;
using serializers::ByteArrayBuffer;
using byte_array::ConstByteArray;

using DagNodes = NaiveSynergeticMiner::DagNodes;

void ExecuteWork(SynergeticContractPtr const &contract, WorkPtr const &work)
{
  WorkScore score{0};

  // execute the work
  auto const nonce_work = work->CreateHashedNonce();

  auto const status = contract->Work(nonce_work, score);

  if (SynergeticContract::Status::SUCCESS != status)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to execute work. Reason: ", ToString(status));

    // set the score to highest possible value
    score = std::numeric_limits<WorkScore>::max();
  }

  // update the score for the piece of work
  work->UpdateScore(score);
  FETCH_LOG_DEBUG(LOGGING_NAME, "Execute Nonce: ", nonce_work.ToHex(), " score: ", score);
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

  state_machine_->OnStateChange([](State /*new_state*/, State /* old_state */) {});
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
  using ProblemSpaces = DigestMap<ProblemData>;

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
      // lookup the problem data
      auto &problem_data = problem_spaces[node.contract_digest];

      // add the problem data to the
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
      oss << "\n -> 0x" << element.first.ToHex();
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

SynergeticContractPtr NaiveSynergeticMiner::LoadContract(Digest const &contract_digest)
{
  SynergeticContractPtr contract{};

  // attempt to retrieve the document stored in the database
  auto const resource_document =
      storage_.Get(SmartContractManager::CreateAddressForSynergeticContract(contract_digest));

  if (!resource_document.failed)
  {
    try
    {
      // create and decode the document buffer
      ByteArrayBuffer buffer{resource_document.document};

      // parse the contents of the document
      ConstByteArray document{};
      buffer >> document;

      // create the instance of the synergetic contract
      contract = std::make_shared<SynergeticContract>(document);
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Error creating contract: ", ex.what());
    }
  }

  return contract;
}

WorkPtr NaiveSynergeticMiner::MineSolution(Digest const &     contract_digest,
                                           ProblemData const &problem_data)
{
  // create the synergetic contract
  auto contract = LoadContract(contract_digest);

  // if no contract can be loaded then simple return
  if (!contract)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup contract: 0x", contract_digest.ToHex());
    return {};
  }

  // build up a work instance
  auto work = std::make_shared<Work>(contract_digest, prover_->identity());

  // Preparing to mine
  auto status = contract->DefineProblem(problem_data);
  if (SynergeticContract::Status::SUCCESS != status)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to define the problem. Reason: ", ToString(status));
    return {};
  }

  // update the initial nonce
  std::random_device rd;
  BigUnsigned        nonce{rd()};

  // generate a series of solutions for each of the problems
  WorkPtr best_work{};
  for (std::size_t i = 0; i < search_length_; ++i)
  {
    // update the nonce
    work->UpdateNonce(nonce);
    ++nonce;

    // execute the work
    ExecuteWork(contract, work);

    // update the cached work if this one is better than previous solutions
    if (!(best_work && best_work->score() >= work->score()))
    {
      best_work = std::make_shared<Work>(*work);
    }
  }

  contract->Detach();

  // Returning the best work from this round
  return best_work;
}

}  // namespace ledger
}  // namespace fetch
