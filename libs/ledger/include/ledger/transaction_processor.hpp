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

#include "ledger/dag/dag_interface.hpp"
#include "ledger/storage_unit/transaction_sinks.hpp"
#include "ledger/transaction_verifier.hpp"

#include <atomic>
#include <memory>
#include <thread>

namespace fetch {
namespace ledger {

class Transaction;
class StorageUnitInterface;
class BlockPackerInterface;
class TransactionStatusCache;

class TransactionProcessor : public TransactionSink
{
public:
  static constexpr char const *LOGGING_NAME = "TransactionProcessor";
  using DAGPtr                              = std::shared_ptr<::fetch::ledger::DAGInterface>;

  // Construction / Destruction
  TransactionProcessor(DAGPtr dag, StorageUnitInterface &storage, BlockPackerInterface &packer,
                       TransactionStatusCache &tx_status_cache, std::size_t num_threads);
  TransactionProcessor(TransactionProcessor const &) = delete;
  TransactionProcessor(TransactionProcessor &&)      = delete;
  ~TransactionProcessor() override;

  /// @name Processor Controls
  /// @{
  void Start();
  void Stop();
  /// @}

  /// @name Transaction Processing
  /// @{
  void AddTransaction(TransactionPtr const &mtx);
  void AddTransaction(TransactionPtr &&mtx);
  /// @}

  // Operators
  TransactionProcessor &operator=(TransactionProcessor const &) = delete;
  TransactionProcessor &operator=(TransactionProcessor &&) = delete;

protected:
  void OnTransaction(TransactionPtr const &tx) override;

private:
  using Flag      = std::atomic<bool>;
  using ThreadPtr = std::unique_ptr<std::thread>;

  DAGPtr                  dag_;
  StorageUnitInterface &  storage_;
  BlockPackerInterface &  packer_;
  TransactionStatusCache &status_cache_;
  TransactionVerifier     verifier_;
  ThreadPtr               poll_new_tx_thread_;
  Flag                    running_{false};

  void ThreadEntryPoint();
};

/**
 * Start the transaction processor
 */
inline void TransactionProcessor::Start()
{
  verifier_.Start();
  running_ = true;
  poll_new_tx_thread_ =
      std::make_unique<std::thread>(&TransactionProcessor::ThreadEntryPoint, this);
}

/**
 * Stop the transactions processor
 */
inline void TransactionProcessor::Stop()
{
  running_ = false;
  if (poll_new_tx_thread_)
  {
    poll_new_tx_thread_->join();
    poll_new_tx_thread_.reset();
  }

  verifier_.Stop();
}

/**
 * Add a single transaction to the processor
 *
 * @param tx The reference to the new transaction to be processed
 */
inline void TransactionProcessor::AddTransaction(TransactionPtr const &tx)
{
  verifier_.AddTransaction(tx);
}

/**
 * Add a single transaction to the processor
 *
 * @param tx The reference to the new transaction to be processed
 */
inline void TransactionProcessor::AddTransaction(TransactionPtr &&tx)
{
  verifier_.AddTransaction(std::move(tx));
}

}  // namespace ledger
}  // namespace fetch
