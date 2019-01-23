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

#include <cstdint>  // for uint32_t, uint64_t
#include <list>

#include "core/mutex.hpp"
#include "ledger/chain/block.hpp"                // for BlockBody, BlockSlice
#include "ledger/chain/main_chain.hpp"           // for MainChain
#include "ledger/chain/mutable_transaction.hpp"  // for TransactionSummary
#include "meta/is_log2.hpp"
#include "miner/miner_interface.hpp"         // for MinerInterface
#include "miner/optimisation/bitvector.hpp"  // for BitVector
#include "vectorise/threading/pool.hpp"      // for Pool

namespace fetch {
namespace miner {

/**
 * Simplistic greedy search algorithm for generating / packing blocks. Has rudimentary support to
 * parallelise the packing over a number of threads.
 *
 * Internally the miner maintains 2 queues. One which is the pending queue which is populated when
 * a new transaction is added to the miner. When block generation begins, this pending queue is
 * transferred to the main queue when it is evaluated in order to generate new blocks. During this
 * operation the main queue is locked.
 */
class BasicMiner : public MinerInterface
{
public:
  static constexpr char const *LOGGING_NAME = "BasicMiner";

  // Construction / Destruction
  explicit BasicMiner(uint32_t log2_num_lanes, uint32_t num_slices);
  BasicMiner(BasicMiner const &) = delete;
  BasicMiner(BasicMiner &&)      = delete;
  ~BasicMiner()                  = default;

  /// @name Miner Interface
  /// @{
  void EnqueueTransaction(chain::TransactionSummary const &tx) override;
  void GenerateBlock(chain::BlockBody &block, std::size_t num_lanes, std::size_t num_slices,
                     chain::MainChain const &chain) override;
  /// @}

  // Operators
  BasicMiner &operator=(BasicMiner const &) = delete;
  BasicMiner &operator=(BasicMiner &&) = delete;

  uint64_t  GetBacklog() const override;
  uint32_t &log2_num_lanes()
  {
    return log2_num_lanes_;
  }

private:
  using BitVector = bitmanip::BitVector;

  struct TransactionEntry
  {
    BitVector                 resources;
    chain::TransactionSummary transaction;

    TransactionEntry(chain::TransactionSummary const &summary, uint32_t log2_num_lanes);
    ~TransactionEntry() = default;
  };

  using Mutex           = mutex::Mutex;
  using TransactionList = std::list<TransactionEntry>;
  using TransactionSet  = std::set<chain::TransactionSummary>;
  using ThreadPool      = threading::Pool;

  static void GenerateSlices(TransactionList &tx, chain::BlockBody &block, std::size_t offset,
                             std::size_t interval, std::size_t num_lanes);
  static void GenerateSlice(TransactionList &tx, chain::BlockSlice &slice, std::size_t slice_index,
                            std::size_t num_lanes);

  static bool SortByFee(TransactionEntry const &a, TransactionEntry const &b);

  uint32_t       log2_num_lanes_;                    ///< The log2 of the number of lanes
  uint32_t const max_num_threads_;                   ///< The configured maximum number of threads
  ThreadPool     thread_pool_;                       ///< The thread pool used to dispatch work
  mutable Mutex  pending_lock_{__LINE__, __FILE__};  ///< The lock for the pending transaction queue
  TransactionList pending_;                          ///< The pending transaction queue
  TransactionSet  txs_seen_;                         ///< The transactions seen so far
  std::size_t     main_queue_size_{0};               ///< The thread safe main queue size
  TransactionList main_queue_;                       ///< The main transaction queue
  bool            filtering_input_duplicates_{
      true};  ///< Whether duplicate transactions are filtered on entry to the packer
};

}  // namespace miner
}  // namespace fetch
