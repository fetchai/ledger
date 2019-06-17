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
#include "ledger/upow/work_queue.hpp"

#include <algorithm>
#include <memory>
#include <queue>

namespace fetch {
namespace ledger {

class WorkPackage
{
public:
  explicit WorkPackage(Digest digest)
    : contract_digest_{std::move(digest)}
  {}
  ~WorkPackage() = default;

  Address const &contract_digest() const
  {
    return contract_digest_;
  }

  WorkQueue const &solutions() const
  {
    return solutions_;
  }

private:
  /// @name defining work and candidate solutions
  /// @{
  Address   contract_digest_{};  //< Contract name which is extracted from the DAG
  WorkQueue solutions_{};        //< A prioritised queue with work items
  /// }
};

using WorkPackagePtr = std::shared_ptr<WorkPackage>;

}  // namespace ledger
}  // namespace fetch
