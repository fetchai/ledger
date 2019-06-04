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

#include "ledger/upow/work.hpp"
#include "ledger/upow/synergetic_miner_interface.hpp"
#include "ledger/upow/synergetic_contract.hpp"

#include <math/bignumber.hpp>
#include <memory>

namespace fetch {
namespace crypto {

class Prover;

}
namespace ledger {

class StorageInterface;
class DAGInterface;
class Address;

class NaiveSynergeticMiner : public SynergeticMinerInterface
{
public:
  using ProverPtr = std::shared_ptr<crypto::Prover>;

  // Construction / Destruction
  NaiveSynergeticMiner(DAGInterface &dag, StorageInterface &storage, ProverPtr prover);
  NaiveSynergeticMiner(NaiveSynergeticMiner const &) = delete;
  NaiveSynergeticMiner(NaiveSynergeticMiner &&) = delete;
  ~NaiveSynergeticMiner() override = default;

  /// @name Synergetic Miner Interface
  /// @{
  DagNodes Mine(BlockIndex block) override;
  /// @}

  // Operators
  NaiveSynergeticMiner &operator=(NaiveSynergeticMiner const &) = delete;
  NaiveSynergeticMiner &operator=(NaiveSynergeticMiner &&) = delete;

private:

  static const std::size_t DEFAULT_SEARCH_LENGTH = 20;

  SynergeticContractPtr LoadContract(Address const &address);
  WorkPtr MineSolution(Address const &address, BlockIndex block);

  DAGInterface     &dag_;
  StorageInterface &storage_;
  ProverPtr         prover_;
  math::BigUnsigned starting_nonce_;
  std::size_t       search_length_{DEFAULT_SEARCH_LENGTH};
};

} // namespace ledger
} // namespace fetch
