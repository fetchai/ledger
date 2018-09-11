#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
#include "core/meta/is_log2.hpp"
#include "miner/miner_interface.hpp"
#include "miner/optimisation/bitvector.hpp"
#include "vectorise/threading/pool.hpp"
#include <list>

namespace fetch {
namespace miner {

/**
 * Simplistic greedy search algorithm for generating / packing blocks. Has rudimentary support to
 * parallelise the packing over a number of threads.
 */
class BasicMiner : public MinerInterface
{
public:

  // Construction / Destruction
  explicit BasicMiner(uint32_t log2_num_lanes, uint32_t num_slices);
  BasicMiner(BasicMiner const &) = delete;
  BasicMiner(BasicMiner &&) = delete;
  ~BasicMiner() = default;

  /// @name Miner Interface
  /// @{
  void EnqueueTransaction(chain::TransactionSummary const &tx) override;
  void GenerateBlock(chain::BlockBody &block, std::size_t num_lanes, std::size_t num_slices) override;
  /// @}

#if 1
  void SetTxPerThread(std::size_t tx_per_thread) { txs_per_thread_ = tx_per_thread; }
#endif

  // Operators
  BasicMiner &operator=(BasicMiner const &) = delete;
  BasicMiner &operator=(BasicMiner &&) = delete;

private:

  using BitVector = bitmanip::BitVector;

  struct TransactionEntry
  {
    BitVector                 resources;
    chain::TransactionSummary transaction;

    TransactionEntry(chain::TransactionSummary const &summary, uint32_t log2_num_lanes);
    ~TransactionEntry() = default;
  };

  using Mutex = mutex::Mutex;
  using TransactionList = std::list<TransactionEntry>;
  using ThreadPool = threading::Pool;

  static void GenerateSlices(TransactionList &tx, chain::BlockBody &block, std::size_t offset, std::size_t interval, std::size_t num_lanes);
  static void GenerateSlice(TransactionList &tx, chain::BlockSlice &slice, std::size_t slice_index, std::size_t num_lanes);

  static bool SortByFee(TransactionEntry const &a, TransactionEntry const &b);

  uint32_t const log2_num_lanes_;
  uint32_t const num_slices_;
  uint32_t const max_num_threads_;

#if 1
  std::size_t txs_per_thread_{0};
#endif

  ThreadPool thread_pool_;

  Mutex pending_lock_{__LINE__, __FILE__};
  TransactionList pending_;

  Mutex main_queue_lock_{__LINE__, __FILE__};
  TransactionList main_queue_;
};

} // namespace miner
} // namespace fetch
