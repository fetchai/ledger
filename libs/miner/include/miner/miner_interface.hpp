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

#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/mutable_transaction.hpp"

#include <cstddef>

namespace fetch {
namespace miner {

/**
 * Interface that generalises all mining / block packing algorithms in the system
 */
class MinerInterface
{
public:
  using BlockBody = ledger::Block::Body;
  using MainChain = ledger::MainChain;

  // Construction / Destruction
  MinerInterface()          = default;
  virtual ~MinerInterface() = default;

  /// @name Miner Interface
  /// @{

  /**
   * Add the specified transaction (summary) to the internal queue
   *
   * @param tx The reference to the transaction
   */
  virtual void EnqueueTransaction(chain::TransactionSummary const &tx) = 0;

  /**
   * Generate a new block based on the current queue of transactions
   *
   * @param block The reference to the output block to generate
   * @param num_lanes The number of lanes for the block
   * @param num_slices The number of slices for the block
   * @param chain The main chain
   */
  virtual void GenerateBlock(BlockBody &block, std::size_t num_lanes, std::size_t num_slices,
                             MainChain const &chain) = 0;

  /**
   * How many transactions are yet to be processed (mem-pool size). Not guaranteed to be accurate.
   *
   * @return: number of transactions
   */
  virtual uint64_t GetBacklog() const = 0;
  /// @}
};

}  // namespace miner
}  // namespace fetch
