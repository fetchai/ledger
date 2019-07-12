#pragma once
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

#include "core/state_machine.hpp"
#include "ledger/dag/dag_interface.hpp"
#include "ledger/upow/synergetic_contract.hpp"
#include "ledger/upow/synergetic_miner_interface.hpp"
#include "ledger/upow/work.hpp"

#include <memory>

namespace fetch {
namespace crypto {

class Prover;
}
namespace ledger {

class StorageInterface;

class NaiveSynergeticMiner : public SynergeticMinerInterface
{
public:
  enum class State
  {
    INITIAL = 0,
    MINE,
  };

  using ProverPtr    = std::shared_ptr<crypto::Prover>;
  using DAGPtr       = std::shared_ptr<ledger::DAGInterface>;
  using StateMachine = core::StateMachine<State>;

  // Construction / Destruction
  NaiveSynergeticMiner(DAGPtr dag, StorageInterface &storage, ProverPtr prover);
  NaiveSynergeticMiner(NaiveSynergeticMiner const &) = delete;
  NaiveSynergeticMiner(NaiveSynergeticMiner &&)      = delete;
  ~NaiveSynergeticMiner() override                   = default;

  /// @name Synergetic Miner Interface
  /// @{
  void Mine() override;
  void EnableMining(bool enable) override;
  /// @}

  // Operators
  NaiveSynergeticMiner &operator=(NaiveSynergeticMiner const &) = delete;
  NaiveSynergeticMiner &operator=(NaiveSynergeticMiner &&) = delete;

  core::WeakRunnable GetWeakRunnable();

private:
  static const std::size_t DEFAULT_SEARCH_LENGTH = 20;

  using ProblemData = ledger::SynergeticContract::ProblemData;

  /// @name State Machine Handlers
  /// @{
  State OnInitial();
  State OnMine();
  /// @}

  /// @name Utils
  /// @{
  SynergeticContractPtr LoadContract(Digest const &contract_digest);
  WorkPtr MineSolution(Digest const &contract_digest, ProblemData const &problem_data);
  /// @}

  DAGPtr                        dag_;
  StorageInterface &            storage_;
  ProverPtr                     prover_;
  std::size_t                   search_length_{DEFAULT_SEARCH_LENGTH};
  std::shared_ptr<StateMachine> state_machine_;
  std::atomic<bool>             is_mining_{false};
};

}  // namespace ledger
}  // namespace fetch
