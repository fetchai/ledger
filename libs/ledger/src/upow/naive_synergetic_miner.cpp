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
#include "ledger/chain/address.hpp"
#include "ledger/upow/naive_synergetic_miner.hpp"
#include "ledger/upow/work.hpp"
#include "ledger/upow/solutions.hpp"

#include "ledger/upow/synergetic_base_types.hpp"
#include "ledger/chaincode/smart_contract_manager.hpp"

#include "vm_modules/math/bignumber.hpp"

#include <unordered_set>

namespace fetch {
namespace ledger {
namespace {

constexpr char const *LOGGING_NAME = "NaiveSynMiner";

using math::BigUnsigned;
using vm_modules::BigNumberWrapper;
using serializers::ByteArrayBuffer;
using byte_array::ConstByteArray;

using AddressSet = std::unordered_set<Address>;
using DagNodes   = NaiveSynergeticMiner::DagNodes;

BigUnsigned GenerateStartingNonce(crypto::Prover const &prover)
{
  Address address{prover.identity()};
  return {address.address()};
}

WorkScore ExecuteWork(SynergeticContractPtr const &contract, WorkPtr const &work)
{
  WorkScore score{0};

  // execute the work
  auto const status = contract->Work(work->CreateHashedNonce(), score);

  if (SynergeticContract::Status::SUCCESS != status)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to execute work. Reason: ", ToString(status));

    // set the score to highest possible value
    score = std::numeric_limits<WorkScore>::max();
  }

  return score;
}

} // namespace

NaiveSynergeticMiner::NaiveSynergeticMiner(DAGPtr dag, StorageInterface &storage, ProverPtr prover)
  : dag_{dag}
  , storage_{storage}
  , prover_{std::move(prover)}
  , starting_nonce_{GenerateStartingNonce(*prover_)}
  , state_machine_{std::make_shared<core::StateMachine<State>>("NaiveSynMiner",
                                                               State::INITIAL)}
{
  state_machine_->RegisterHandler(State::INITIAL, this, &NaiveSynergeticMiner::OnInitial);
  state_machine_->RegisterHandler(State::MINE, this, &NaiveSynergeticMiner::OnMine);

  state_machine_->OnStateChange([](State /*new_state*/, State /* old_state */) { });
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

  this->Mine(0);

  return State::INITIAL;
}

DagNodes NaiveSynergeticMiner::Mine(BlockIndex block)
{
  // iterate through the latest DAG nodes and build a complete set of addresses to mine solutions
  // for
  // TODO(HUT): would be nicer to specify here what we want by type
  auto dag_nodes = dag_->GetLatest();

  // Debug
  {
    AddressSet address_set;
    for (auto const &node : dag_nodes)
    {
      if (DAGNode::DATA == node.type)
      {
        assert(!node.contract_digest.empty());
        address_set.insert(node.contract_digest);
      }
    }

    if (address_set.empty())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "No data to be mined");
      return {};
    }
    else
    {
      std::ostringstream oss;
      for (auto const &address : address_set)
      {
        oss << '\n' << address.display();
      }

      FETCH_LOG_INFO(LOGGING_NAME, "Available synergetic contracts to be mined", oss.str());
    }
  }

  // for each of the contract addresses available mine a solution
  for (auto const &dag_node : dag_nodes)
  {
    if (DAGNode::DATA == dag_node.type)
    {
      auto const solution = MineSolution(dag_node.contract_digest, block);

      if (solution)
      {
        // TODO(HUT): get signing correct
        dag_->AddWork(*solution);

        FETCH_LOG_INFO(LOGGING_NAME, "Mined and added work! Epoch number: ", dag_->CurrentEpoch());
      }
    }
  }

  return {};
}

SynergeticContractPtr NaiveSynergeticMiner::LoadContract(Address const &address)
{
  SynergeticContractPtr contract{};

  // attempt to retrieve the document stored in the database
  auto const resource_document =
      storage_.Get(SmartContractManager::CreateAddressForContract(address));

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

WorkPtr NaiveSynergeticMiner::MineSolution(Address const &address, BlockIndex block)
{
  // create the synergetic contract
  auto contract = LoadContract(address);

  // if no contract can be loaded then simple return
  if (!contract)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup contract: ", address.display());
    return {};
  }

  // build up a work instance
  auto work = std::make_shared<Work>(address, prover_->identity(), block);

  // Preparing to mine
  auto status = contract->DefineProblem();
  if (SynergeticContract::Status::SUCCESS != status)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to define the problem. Reason: ", ToString(status));
    return {};
  }

  // update the initial nonce
  BigUnsigned nonce{starting_nonce_.Copy()};

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

} // namespace ledger
} // namespace fetch

