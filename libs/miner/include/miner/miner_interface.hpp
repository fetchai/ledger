#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include "ledger/chain/block.hpp"
#include "ledger/chain/mutable_transaction.hpp"

namespace fetch {
namespace miner {

class MinerInterface
{
public:
  // Construction / Destruction
  MinerInterface()          = default;
  virtual ~MinerInterface() = default;

  /// @name Miner Interface
  /// @{
  virtual void EnqueueTransaction(chain::TransactionSummary const &tx) = 0;
  virtual void GenerateBlock(chain::BlockBody &block, std::size_t num_lanes,
                             std::size_t num_slices)                   = 0;
  /// @}
};

}  // namespace miner
}  // namespace fetch
