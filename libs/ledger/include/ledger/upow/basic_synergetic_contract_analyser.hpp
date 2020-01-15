#pragma once
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

#include "ledger/upow/synergetic_contract_analyser_interface.hpp"

#include <cstdint>
#include <vector>

namespace fetch {
namespace ledger {

class StorageInterface;


class BasicSynergeticContractAnalyser : public SynergeticContractAnalyserInterface
{
public:
  using Variant     = SynergeticContractAnalyserInterface::Variant;
  using ProblemData = SynergeticContractAnalyserInterface::ProblemData;

  // Construction / Destruction
  BasicSynergeticContractAnalyser(StorageInterface& storage, crypto::Identity miner, std::size_t num_lanes);
  virtual ~BasicSynergeticContractAnalyser() = default;

  Variant AnalyseContract(chain::Address const &contract_address, ProblemData const &problem_data) override;

private:
  StorageInterface  &storage_;
  crypto::Identity miner_;
  std::size_t num_lanes_;


  std::unique_ptr<SynergeticContract> GetContract(chain::Address const &contract_address);

};

}  // namespace ledger
}  // namespace fetch
