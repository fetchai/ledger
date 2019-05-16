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

#include "core/mutex.hpp"
#include "ledger/block_packer_interface.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/transaction.hpp"
#include "meta/log2.hpp"
#include "vectorise/threading/pool.hpp"
#include "ledger/chain/v2/transaction_layout.hpp"
#include "ledger/chain/v2/digest.hpp"

#include "miner/transaction_layout_queue.hpp"

#include <list>

namespace fetch {
namespace miner {

/**
 * Simplistic greedy search algorithm for generating / packing blocks. Has rudimentary support to
 * parallelize the packing over a number of threads.
 *
 * Internally the miner maintains 2 queues. One which is the pending queue which is populated when
 * a new transaction is added to the miner. When block generation begins, this pending queue is
 * transferred to the main queue when it is evaluated in order to generate new blocks. During this
 * operation the main queue is locked.
 */
class BasicMiner : public ledger::BlockPackerInterface
{
public:
  static constexpr char const *LOGGING_NAME = "BasicMiner";

  using Block             = ledger::Block;
  using MainChain         = ledger::MainChain;
  using TransactionLayout = ledger::v2::TransactionLayout;

  // Construction / Destruction
  explicit BasicMiner(uint32_t log2_num_lanes);
  BasicMiner(BasicMiner const &) = delete;
  BasicMiner(BasicMiner &&)      = delete;
  ~BasicMiner() override         = default;

  /// @name Miner Interface
  /// @{
  void EnqueueTransaction(ledger::v2::Transaction const &tx) override;
  void EnqueueTransaction(ledger::v2::TransactionLayout const &layout) override;
  void GenerateBlock(Block &block, std::size_t num_lanes, std::size_t num_slices,
                     MainChain const &chain) override;
  uint64_t GetBacklog() const override;
  /// @}

  // Operators
  BasicMiner &operator=(BasicMiner const &) = delete;
  BasicMiner &operator=(BasicMiner &&) = delete;

private:
  using Mutex           = mutex::Mutex;
  using TransactionList = TransactionLayoutQueue::UnderlyingList;
  using TransactionSet  = std::unordered_set<ledger::v2::TransactionLayout>;
  using ThreadPool      = threading::Pool;
  using DigestSet       = ledger::v2::DigestSet;
  using Queue           = TransactionLayoutQueue;

  /// @name Packing Operations
  /// @{
  static void GenerateSlices(Queue &transactions, Block::Body &block, std::size_t offset,
                             std::size_t interval, std::size_t num_lanes);
  static void GenerateSlice(Queue &transactions, Block::Slice &slice, std::size_t slice_index,
                            std::size_t num_lanes);
  static bool SortByFee(TransactionLayout const &a, TransactionLayout const &b);
  /// @}

  /// @name Configuration
  /// @{
  uint32_t       log2_num_lanes_;                    ///< The log2 of the number of lanes
  uint32_t const max_num_threads_;                   ///< The configured maximum number of threads
  ThreadPool     thread_pool_;                       ///< The thread pool used to dispatch work
  /// @}

  /// @name Pending Queue
  /// @{
  mutable Mutex pending_lock_{__LINE__, __FILE__};      ///< Pending queue lock (priority 1)
  Queue         pending_;               ///< The main mining queue for the node
  /// @}

  /// @name Central Mining Pool Queue
  /// @{
  mutable Mutex mining_pool_lock_{__LINE__, __FILE__};  ///< Mining pool lock (priority 0)
  Queue         mining_pool_;               ///< The main mining queue for the node
  /// @}
};

}  // namespace miner
}  // namespace fetch
