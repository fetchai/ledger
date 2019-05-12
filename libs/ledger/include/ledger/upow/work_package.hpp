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

#include "ledger/identifier.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "ledger/upow/work.hpp"

#include <algorithm>
#include <memory>
#include <queue>
namespace fetch {
namespace consensus {

struct WorkPackage
{
  using ContractName      = byte_array::ConstByteArray;
  using ContractAddress   = storage::ResourceAddress;
  using SharedWorkPackage = std::shared_ptr<WorkPackage>;
  using WorkQueue         = std::priority_queue<Work>;

  static SharedWorkPackage New(ContractName name, ContractAddress contract_address)
  {
    return SharedWorkPackage(new WorkPackage(std::move(name), std::move(contract_address)));
  }

  /// @name defining work and candidate solutions
  /// @{
  ContractName    contract_name{};     //< Contract name which is extracted from the DAG
  ContractAddress contract_address{};  //< The corresponding address of the contract
  WorkQueue       solutions_queue{};   //< A prioritised queue with work items
  /// }

  // Work packages needs to be sortable to ensure consistent execution
  // order accross different platforms.
  bool operator<(WorkPackage const &other) const
  {
    return contract_address < other.contract_address;
  }

private:
  WorkPackage(ContractName name, ContractAddress address)
    : contract_name{std::move(name)}
    , contract_address{std::move(address)}
  {}
};

}  // namespace consensus
}  // namespace fetch