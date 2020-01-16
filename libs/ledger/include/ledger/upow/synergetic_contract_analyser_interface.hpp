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

#include "ledger/upow/synergetic_contract.hpp"
#include "variant/variant.hpp"
#include "ledger/upow/synergetic_job.hpp"

#include <cstdint>
#include <vector>

namespace fetch {
namespace ledger {

class SynergeticContractAnalyserInterface
{
public:
  using SynergeticJobPtr = std::unique_ptr<SynergeticJob>;
  using ProblemData      = ledger::SynergeticContract::ProblemData;

  // Construction / Destruction
  SynergeticContractAnalyserInterface()          = default;
  virtual ~SynergeticContractAnalyserInterface() = default;

  /// @name Contract Analyser Interface
  /// @{
  virtual SynergeticJobPtr AnalyseContract(chain::Address const &contract_address, ProblemData const &problem_data,
      uint64_t const &charge_limit) = 0;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
