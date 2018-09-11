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

#include "miner/basic_miner.hpp"
#include "miner/resource_mapper.hpp"

#include <algorithm>

#if 1
#include "core/byte_array/decoders.hpp"
#include <iostream>
#endif

namespace fetch {
namespace miner {
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

/**
 * Calculate the max number of threads given the hardward and the number of slices
 *
 * @param num_slices The number of slices for the block
 * @return The maximum number of threads
 */
uint32_t CalculateMaxNumThreads(uint32_t num_slices)
{
  return Clip3(num_slices / 128u, 1u, std::thread::hardware_concurrency());
}

} // namespace

/**
 * Construct a transaction entry for the queue
 *
 * @param summary The reference to the transaction
 * @param log2_num_lanes The log2 of the number of lanes
 */
BasicMiner::TransactionEntry::TransactionEntry(chain::TransactionSummary const &summary, uint32_t log2_num_lanes)
  : resources{1u << log2_num_lanes}
  , transaction{summary}

{
  // update the resources array with the correct bits flags for the lanes
  for (auto const &resource : summary.resources)
  {
    // map the resource to a lane
    uint32_t const lane = MapResourceToLane(resource, summary.contract_name, log2_num_lanes);

    // update the bit flag
    resources.set(lane, 1);
  }
}

/**
 * Construct the BasicMiner
 *
 * @param log2_num_lanes Log2 of the number of lanes
 * @param num_slices The number of slices
 */
BasicMiner::BasicMiner(uint32_t log2_num_lanes, uint32_t num_slices)
  : log2_num_lanes_{log2_num_lanes}
  , max_num_threads_{CalculateMaxNumThreads(num_slices)}
  , thread_pool_{max_num_threads_}
{
}

/**
 * Add the specified transaction (summary) to the internal queue
 *
 * @param tx The reference to the transaction
 */
void BasicMiner::EnqueueTransaction(chain::TransactionSummary const &tx)
{
  FETCH_LOCK(pending_lock_);
  pending_.emplace_back(tx, log2_num_lanes_);
}

/**
 * Generate a new block based on the current queue of transactions
 *
 * @param block The reference to the output block to generate
 * @param num_lanes The number of lanes for the block
 * @param num_slices The number of slices for the block
 */
void BasicMiner::GenerateBlock(chain::BlockBody &block, std::size_t num_lanes, std::size_t num_slices)
{
  assert(num_lanes == (1u << log2_num_lanes_));

  FETCH_LOCK(main_queue_lock_);

  // add the entire contents of the
  {
    FETCH_LOCK(pending_lock_);
    main_queue_.splice(main_queue_.end(), pending_);
  }

  // determine how many of the threads should be used in this block generation
  std::size_t num_threads = Clip3<std::size_t>(main_queue_.size() / txs_per_thread_, 1u, max_num_threads_);

#if 1
  if (txs_per_thread_)
  {
    num_threads = std::min<std::size_t>(txs_per_thread_, max_num_threads_);
  }
#endif

  // prepare the basic formatting for the block
  block.slices.resize(num_slices);

  // skip thread generation in the simple case
  if (num_threads == 1)
  {
#if 1
    std::cout << "Num Threads: " << num_threads << std::endl;
    std::cout << "Num Slice..: " << num_slices << std::endl;
    std::cout << "Num TX.....: " << main_queue_.size() << std::endl;
#endif

    GenerateSlices(main_queue_, block, 0, 1, num_lanes);
  }
  else
  {
    // split the main queue into a series of smaller lists
    std::vector<TransactionList> transaction_lists(num_threads);

    std::size_t const num_slices_per_thread = num_slices / num_threads;
    std::size_t const num_tx_per_thread = main_queue_.size() / num_threads;

#if 1
    std::cout << "Num Threads: " << num_threads << std::endl;
    std::cout << "Num Slice..: " << num_slices_per_thread << std::endl;
    std::cout << "Num TX.....: " << num_tx_per_thread << std::endl;
#endif

    for (std::size_t i = 0; i < num_threads; ++i)
    {
      TransactionList &txs = transaction_lists[i];

      auto start = main_queue_.begin();
      auto end   = start;
      std::advance(end, static_cast<int32_t>(std::min(num_tx_per_thread, main_queue_.size())));

      // splice in the contents of the array
      txs.splice(txs.end(), main_queue_, start, end);

      thread_pool_.Dispatch([&txs, &block, i, num_threads, num_lanes]() {
        GenerateSlices(txs, block, i, num_threads, num_lanes);
      });
    }

    // wait for all the threads to complete
    thread_pool_.Wait();

    // return all the transactions to the main queue
    for (std::size_t i = 0; i < num_threads; ++i)
    {
      main_queue_.splice(main_queue_.begin(), transaction_lists[i]);
    }
  }
}

/**
 * Internal: Generate a selection of slices
 *
 * @param tx The reference to the transaction list to be used when generating the selection of slices
  * @param block The reference to the block to populate
 * @param offset The slice index offset to start from
 * @param interval The slice index interval to be used when selecting the next slice to populate
 * @param num_lanes The number of lanes of the block
 */
void BasicMiner::GenerateSlices(TransactionList &tx, chain::BlockBody &block, std::size_t offset, std::size_t interval, std::size_t num_lanes)
{
  // sort the transaction list by fees
  tx.sort(SortByFee);

  for (std::size_t slice_idx = offset; slice_idx < block.slices.size(); slice_idx += interval)
  {
    auto &slice = block.slices[slice_idx];

    // generate the slice
    GenerateSlice(tx, slice, slice_idx, num_lanes);
  }
}

/**
 * Internal: Generate a slice
 *
 * @param tx The reference to the transaction list to be used when generating the slice
 * @param slice The slice to be populated
 * @param slice_index The slice number
 * @param num_lanes The number of lanes for the block
 */
void BasicMiner::GenerateSlice(TransactionList &tx, chain::BlockSlice &slice, std::size_t slice_index, std::size_t num_lanes)
{
  BitVector slice_state{num_lanes};

  auto it = tx.begin();
  while (it != tx.end())
  {
    // exit the search loop once the slice is full
    if (slice_state.PopCount() == num_lanes)
      break;

    // calculate the collisions for this
    BitVector const collisions = slice_state & it->resources;

    // determine if there are collisions
    if (collisions.PopCount() == 0)
    {
      // update the slice state
      slice_state |= it->resources;

      // insert the transaction into the slice
      slice.transactions.push_back(it->transaction);

      // remove the transaction from the main queue
      it = tx.erase(it);
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
 * @param b The refernece to the other transaction entry
 * @return true if a is preferable to b, otherwise false
 */
bool BasicMiner::SortByFee(TransactionEntry const &a, TransactionEntry const &b)
{
  return a.transaction.fee > b.transaction.fee;
}

} // namespace miner
} // namespace fetch
