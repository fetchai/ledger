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

#include "ledger/chain/digest.hpp"
#include "ledger/execution_result.hpp"

namespace fetch {

class BitVector;

namespace ledger {

class Address;

class ExecutorInterface
{
public:
  using BlockIndex  = uint64_t;
  using SliceIndex  = uint64_t;
  using LaneIndex   = uint32_t;
  using TokenAmount = uint64_t;
  using Status      = ContractExecutionStatus;
  using Result      = ContractExecutionResult;

  // Construction / Destruction
  ExecutorInterface()          = default;
  virtual ~ExecutorInterface() = default;

  /// @name Executor Interface
  /// @{
  virtual Result Execute(Digest const &digest, BlockIndex block, SliceIndex slice,
                         BitVector const &shards)                                              = 0;
  virtual void   SettleFees(Address const &miner, TokenAmount amount, uint32_t log2_num_lanes) = 0;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
