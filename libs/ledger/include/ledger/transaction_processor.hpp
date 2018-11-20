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

#include "core/containers/queue.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "metrics/metrics.hpp"
#include "miner/miner_interface.hpp"
#include "network/details/thread_pool.hpp"
#include "ledger/storage_unit/transaction_sinks.hpp"

#include <atomic>
#include <thread>

namespace fetch {
namespace ledger {

class TransactionProcessor : public UnverifiedTransactionSink
{
public:
  using MutableTransaction = chain::MutableTransaction;

  using TransactionList = std::vector<chain::Transaction>;

  // Construction / Destruction
  TransactionProcessor(StorageUnitInterface &storage, miner::MinerInterface &miner);
  TransactionProcessor(TransactionProcessor const &) = delete;
  TransactionProcessor(TransactionProcessor &&)      = delete;
  ~TransactionProcessor()                            = default;

  /// @name Processor Controls
  /// @{
  void Start();
  void Stop();
  /// @}

  /// @name Transaction Processing
  /// @{
  void AddTransaction(MutableTransaction const &mtx);
  void AddTransaction(MutableTransaction &&mtx);
  /// @}

  /// @name Unverified Transaction Sink
  /// @{
  void OnTransaction(chain::UnverifiedTransaction &&tx) override;
  /// @}

  // Operators
  TransactionProcessor &operator=(TransactionProcessor const &) = delete;
  TransactionProcessor &operator=(TransactionProcessor &&) = delete;

private:
  static constexpr std::size_t QUEUE_SIZE = 1u << 20u;  // 1,048,576

  using Flag            = std::atomic<bool>;
  using VerifiedQueue   = core::SimpleQueue<chain::VerifiedTransaction, QUEUE_SIZE>;
  using UnverifiedQueue = core::MPMCQueue<chain::MutableTransaction, QUEUE_SIZE>;
  using ThreadPtr       = std::unique_ptr<std::thread>;
  using Threads         = std::vector<ThreadPtr>;

  void Verifier();
  void Dispatcher();

  StorageUnitInterface & storage_;
  miner::MinerInterface &miner_;

  Flag            active_{true};
  Threads         threads_;
  VerifiedQueue   verified_queue_;
  UnverifiedQueue unverified_queue_;
};

/**
 * Add a single transaction to the processor
 *
 * @param tx The reference to the new transaction to be processed
 */
inline void TransactionProcessor::AddTransaction(MutableTransaction const &mtx)
{
  // enqueue the transaction to the unverified queue
  unverified_queue_.Push(mtx);
}

/**
 * Add a single transaction to the processor
 *
 * @param tx The reference to the new transaction to be processed
 */
inline void TransactionProcessor::AddTransaction(MutableTransaction &&mtx)
{
  // enqueue the transaction to the unverified queue
  unverified_queue_.Push(std::move(mtx));
}

}  // namespace ledger
}  // namespace fetch
