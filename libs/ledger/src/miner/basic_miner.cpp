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

#include "chain/address.hpp"
#include "chain/transaction.hpp"
#include "chain/transaction_validity_period.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/miner/basic_miner.hpp"
#include "logging/logging.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/registry.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <thread>
#include <vector>

namespace fetch {
namespace ledger {
namespace {

/**
 * Clip the specified to the bounds: [min_value, max_value]
 *
 * @tparam T The type of the value
 * @param value The input value to be clipped
 * @param min_value The minimum (inclusive) value
 * @param max_value The maximum (inclusive) value
 * @return The clipped value
 */
template <typename T>
T Clip3(T value, T min_value, T max_value)
{
  return std::min(std::max(value, min_value), max_value);
}

}  // namespace

/**
 * Construct the BasicMiner
 *
 * @param log2_num_lanes Log2 of the number of lanes
 * @param num_slices The number of slices
 */
BasicMiner::BasicMiner(uint32_t log2_num_lanes)
  : log2_num_lanes_{log2_num_lanes}
  , max_num_threads_{std::thread::hardware_concurrency()}
  , thread_pool_{max_num_threads_, "Miner"}
  , mining_pool_size_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "ledger_miner_mining_pool_size", "The current size of the mining pool")}
  , max_mining_pool_size_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "ledger_miner_max_mining_pool_size", "The max size of the mining pool")}
  , max_pending_pool_size_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "ledger_miner_max_pending_pool_size", "The max size of the pending pool")}
  , duplicate_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_miner_duplicate_total", "The number of duplicate txs on the frontend of the queue")}
  , duplicate_filtered_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_miner_duplicate_filtered_total",
        "The number of duplicate txs on the backend of the queue")}
{}

/**
 * Add the specified transaction (summary) to the internal queue
 *
 * @param tx The reference to the transaction
 */
void BasicMiner::EnqueueTransaction(chain::Transaction const &tx)
{
  EnqueueTransaction(chain::TransactionLayout{tx, log2_num_lanes_});
}

/**
 * Add the specified transaction layout to the internal queue
 *
 * This method is distinct from the case above since it allows the miner to pack the transaction
 * into a block before actually receiving the complete transaction payload.
 *
 * @param layout The layout to be added to the queue
 */
void BasicMiner::EnqueueTransaction(chain::TransactionLayout const &layout)
{
  FETCH_LOCK(pending_lock_);

  if (layout.mask().size() != (1u << log2_num_lanes_))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Discarding layout due to incompatible mask size");
    return;
  }

  if (pending_.Add(layout))
  {
    max_pending_pool_size_->max(pending_.size());
    FETCH_LOG_DEBUG(LOGGING_NAME, "Enqueued Transaction (added) 0x", layout.digest().ToHex());
  }
  else
  {
    duplicate_count_->increment();
    FETCH_LOG_DEBUG(LOGGING_NAME, "Enqueued Transaction (duplicate) 0x", layout.digest().ToHex());
  }
}

/**
 * Generate a new block based on the current queue of transactions. Not thread safe.
 *
 * @param block The reference to the output block to generate
 * @param num_lanes The number of lanes for the block
 * @param num_slices The number of slices for the block
 */
void BasicMiner::GenerateBlock(Block &block, std::size_t num_lanes, std::size_t num_slices,
                               MainChain const &chain)
{
  FETCH_LOCK(mining_pool_lock_);
  assert(num_lanes == (1u << log2_num_lanes_));

  // splice the contents of the pending queue into the main mining pool
  {
    FETCH_LOCK(pending_lock_);
    mining_pool_.Splice(pending_);
  }

  std::remove_if(mining_pool_.begin(), mining_pool_.end(), [&block](auto const &tx_layout) {
    return fetch::chain::GetValidity(tx_layout, block.block_number) !=
           chain::Transaction::Validity::VALID;
  });

  // detect the transactions which have already been incorporated into previous blocks
  auto const duplicates =
      chain.DetectDuplicateTransactions(block.previous_hash, mining_pool_.TxLayouts());

  duplicate_filtered_count_->add(duplicates.size());

  // remove the duplicates
  mining_pool_.Remove(duplicates);

  mining_pool_size_->set(mining_pool_.size());
  max_mining_pool_size_->max(mining_pool_.size());

  // cache the size of the mining pool
  std::size_t const pool_size_before = mining_pool_.size();

  FETCH_LOG_INFO(LOGGING_NAME, "Starting block packing. Pool Size: ", pool_size_before);

  // determine how many of the threads should be used in this block generation
  auto const num_threads = Clip3<std::size_t>(mining_pool_.size() / 1000u, 1u, max_num_threads_);

  // prepare the basic formatting for the block
  block.slices.resize(num_slices);

  // skip thread generation in the simple case
  if (num_threads == 1)
  {
    GenerateSlices(mining_pool_, block, 0, 1, num_lanes);
  }
  else if (num_threads > 1)
  {
    // split the main queue into a series of smaller queues that can be executed in parallel
    std::vector<Queue> transaction_lists(num_threads);

    std::size_t const num_tx_per_thread = mining_pool_.size() / num_threads;

    for (std::size_t i = 0; i < num_threads; ++i)
    {
      Queue &txs = transaction_lists[i];

      auto start = mining_pool_.begin();
      auto end   = start;
      std::advance(end, static_cast<int32_t>(std::min(num_tx_per_thread, mining_pool_.size())));

      // TODO(private issue XXX): While this efficiently breaks up the transaction queue into even
      //  chunks it assumes that the fees of the transactions are roughly the same. This algorithm
      //  should be updated to maximise collected fees for the miner.

      // splice in the contents of the array
      txs.Splice(mining_pool_, start, end);

      thread_pool_.Dispatch([&txs, &block, i, num_threads, num_lanes]() {
        GenerateSlices(txs, block, i, num_threads, num_lanes);
      });
    }

    // wait for all the threads to complete
    thread_pool_.Wait();

    // return all the transactions to the main queue
    for (std::size_t i = 0; i < num_threads; ++i)
    {
      mining_pool_.Splice(transaction_lists[i]);
    }
  }

  block.UpdateTimestamp();

  std::size_t const remaining_transactions = mining_pool_.size();
  std::size_t const packed_transactions    = pool_size_before - remaining_transactions;

  FETCH_LOG_INFO(LOGGING_NAME, "Finished block packing (packed: ", packed_transactions,
                 " remaining: ", remaining_transactions, ")");
}

/**
 * Get the number of transactions that make up the mining pool
 *
 * @return The number of pending transactions
 */
uint64_t BasicMiner::GetBacklog() const
{
  FETCH_LOCK(mining_pool_lock_);
  return mining_pool_.size();
}

/**
 * Internal: Generate a selection of slices
 *
 * @param transactions The transaction queue to be used when generating the selection of slices
 * @param block The reference to the block to populate
 * @param offset The slice index offset to start from
 * @param interval The slice index interval to be used when selecting the next slice to populate
 * @param num_lanes The number of lanes of the block
 */
void BasicMiner::GenerateSlices(Queue &transactions, Block &block, std::size_t offset,
                                std::size_t interval, std::size_t num_lanes)
{
  // sort by fees
  transactions.Sort(SortByFee);

  for (std::size_t slice_idx = offset; slice_idx < block.slices.size(); slice_idx += interval)
  {
    auto &slice = block.slices[slice_idx];

    // generate the slice
    GenerateSlice(transactions, slice, slice_idx, num_lanes);
  }
}

/**
 * Internal: Generate a slice
 *
 * @param transactions The transaction queue to be used when generating the slice
 * @param slice The slice to be populated
 * @param slice_index The slice number
 * @param num_lanes The number of lanes for the block
 */
void BasicMiner::GenerateSlice(Queue &transactions, Block::Slice &      slice,
                               std::size_t /*slice_index*/, std::size_t num_lanes)
{
  BitVector slice_state{num_lanes};

  auto it = transactions.begin();
  while (it != transactions.end())
  {
    // exit the search loop once the slice is full
    if (slice_state.PopCount() == num_lanes)
    {
      break;
    }

    BitVector const &mask = it->mask();

    // calculate the collisions for this
    BitVector const collisions = slice_state & mask;

    // determine if there are collisions
    if (collisions.PopCount() == 0)
    {
      // update the slice state
      slice_state |= mask;

      // insert the transaction into the slice
      slice.push_back(*it);

      // remove the transaction from the main queue
      it = transactions.Erase(it);
    }
    else
    {
      // advance to the next iterator
      ++it;
    }
  }
}

/**
 * Sorting method for transactions
 *
 * @param a The reference to a transaction entry
 * @param b The reference to the other transaction entry
 * @return true if a is preferable to b, otherwise false
 */
bool BasicMiner::SortByFee(TransactionLayout const &a, TransactionLayout const &b)
{
  // this doesn't seem to the a good metric
  return a.charge_rate() > b.charge_rate();
}

}  // namespace ledger
}  // namespace fetch
