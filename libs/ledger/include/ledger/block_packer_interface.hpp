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

#include <cstddef>
#include <cstdint>

namespace fetch {
namespace ledger {

namespace v2 {

class Transaction;
class TransactionLayout;

}

// forward declarations
class Block;
class MainChain;
struct TransactionSummary;

/**
 * Interface that generalises all mining / block packing algorithms in the system
 */
class BlockPackerInterface
{
public:
  // Construction / Destruction
  BlockPackerInterface()          = default;
  virtual ~BlockPackerInterface() = default;

  /// @name Miner Interface
  /// @{

  /**
   * Add the specified transaction to the internal queue
   *
   * @param tx The reference to the transaction
   */
  virtual void EnqueueTransaction(v2::Transaction const &tx) = 0;

  /**
   * Add the specified transaction layout to the internal queue
   *
   * This method is distinct from the case above since it allows the miner to pack the transaction
   * into a block before actually receiving the complete transaction payload.
   *
   * @param layout The layout to be added to the queue
   */
  virtual void EnqueueTransaction(v2::TransactionLayout const &layout) = 0;

  /**
   * Generate a new block based on the current queue of transactions
   *
   * @param block The reference to the output block to generate
   * @param num_lanes The number of lanes for the block
   * @param num_slices The number of slices for the block
   * @param chain The main chain
   */
  virtual void GenerateBlock(Block &block, std::size_t num_lanes, std::size_t num_slices,
                             MainChain const &chain) = 0;

  /**
   * How many transactions are yet to be processed (mem-pool size). Not guaranteed to be accurate.
   *
   * @return: number of transactions
   */
  virtual uint64_t GetBacklog() const = 0;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
