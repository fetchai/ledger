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
#include "crypto/fnv.hpp"
#include "miner/block_optimiser.hpp"
#include "miner/miner_interface.hpp"
#include "miner/transaction_item.hpp"

#include <iostream>

namespace fetch {
namespace miner {

class AnnealerMiner : public MinerInterface
{
public:
  using transaction_type       = std::shared_ptr<TransactionItem>;
  using mutex_type             = fetch::mutex::Mutex;
  using lock_guard_type        = std::lock_guard<mutex_type>;
  using transaction_queue_type = std::vector<transaction_type>;
  using generator_type         = chain::BlockGenerator;

  static constexpr char const *LOGGING_NAME = "AnnealerMiner";

  void EnqueueTransaction(chain::TransactionSummary const &tx) override
  {
    lock_guard_type lock(pending_queue_lock_);

    auto stx = std::make_shared<TransactionItem>(tx, transaction_index_++);

    FETCH_LOG_DEBUG(LOGGING_NAME,"EnqueueTransaction: ", byte_array::ToBase64(tx.transaction_hash));
    pending_queue_.push_back(stx);
  }

  void GenerateBlock(chain::BlockBody &block, std::size_t num_lanes,
                     std::size_t num_slices) override
  {

    // push the currently pending elements into the generator
    {
      lock_guard_type lock(pending_queue_lock_);

      for (auto &tx : pending_queue_)
      {
        FETCH_LOG_DEBUG(LOGGING_NAME,"Pushing Transaction: ", byte_array::ToBase64(tx->summary().transaction_hash));
        generator_.PushTransactionSummary(tx);
      }
      pending_queue_.clear();
    }

    std::size_t const num_transactions = generator_.unspent_count();

    FETCH_LOG_DEBUG(LOGGING_NAME,"Transaction Backlog: ", num_transactions);

    if (num_transactions == 1)
    {
      FillBlock(block, num_lanes, num_slices);
    }
    else if (num_transactions > 1)
    {
      PopulateBlock(block, num_lanes, num_slices);
    }

#if 0
    // debugging interface
    if (num_transactions > 0)
    {
      FETCH_LOG_INFO(LOGGING_NAME,"Block Structure: ");
      std::size_t slice_index = 0;
      for (auto const &slice : block.slices)
      {
        FETCH_LOG_INFO(LOGGING_NAME,"  - Slice ", slice_index);

        for (auto const &tx : slice.transactions)
        {
          FETCH_LOG_INFO(LOGGING_NAME,"    - Tx: ", byte_array::ToBase64(tx.transaction_hash));
        }

        ++slice_index;
      }
    }
#endif
  }

private:
  void FillBlock(chain::BlockBody &block, std::size_t num_lanes, std::size_t num_slices)
  {
    block.slices.clear();
    block.slices.resize(num_slices);

    auto &unspent = generator_.unspent();

    for (auto const &tx : unspent)
    {
      block.slices[0].transactions.push_back(tx->summary());
    }

    unspent.clear();
  }

  void PopulateBlock(chain::BlockBody &block, std::size_t num_lanes, std::size_t num_slices)
  {

    // configure the solver
    generator_.ConfigureAnnealer(100, 0.1, 3.0);

    // TODO(issue 7):  Move to configuration variables
    std::size_t const batch_size  = generator_.unspent_count();
    std::size_t const repetitions = 1000u;
    std::size_t const explore     = 10;
    std::size_t const strategy    = 0;

    // Generating solution
    for (std::size_t i = 0; i < repetitions; ++i)
    {
      generator_.Reset();
      generator_.GenerateBlock(num_lanes, num_slices, strategy, batch_size, explore);
    }

    // DEBUG PRINT OUT SOLUTIONS

    uint64_t total_fee = 0;
    for (auto const &e : generator_.block_fees()) total_fee += e;

    std::cout << "Fee: " << total_fee << std::endl;
    std::cout << "Num Staged: " << generator_.staged().size() << std::endl;

    auto &staged = generator_.staged();

    block.slices.clear();
    block.slices.resize(num_slices);

    detailed_assert(block.slices.size() == staged.size());
    std::size_t slice_index = 0;
    for (auto const &slice : staged)
    {
      for (auto const &tx : slice)
      {
        block.slices[slice_index].transactions.push_back(tx->summary());
      }
      ++slice_index;
    }

    staged.clear();
  }

  mutex_type pending_queue_lock_{__LINE__, __FILE__};  ///< Protects both `pending_queue_` and
                                                       ///< `transaction_index_`
  transaction_queue_type pending_queue_;
  std::size_t            transaction_index_{0};

  generator_type generator_;
};

}  // namespace miner
}  // namespace fetch
