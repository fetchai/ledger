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

#include "ledger/upow/work_queue.hpp"

namespace fetch {
namespace ledger {

class SynergeticExecutorInterface
{
public:
  using ConstByteArray = byte_array::ConstByteArray;
  using ProblemData    = std::vector<ConstByteArray>;

  // Construction / Destruction
  SynergeticExecutorInterface()          = default;
  virtual ~SynergeticExecutorInterface() = default;

  /// @name Executor Interface
  /// @{
  virtual void Verify(WorkQueue &solutions, ProblemData const &problem_data, std::size_t num_lanes,
                      chain::Address const &miner) = 0;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
