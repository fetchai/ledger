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

#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "ledger/storage_unit/transaction_sinks.hpp"
#include "ledger/transaction_verifier.hpp"
#include "miner/miner_interface.hpp"

#include <atomic>
#include <thread>

namespace fetch {
namespace ledger {

class TransactionProcessor : public UnverifiedTransactionSink, public VerifiedTransactionSink
{
public:
  using MutableTransaction = chain::MutableTransaction;
  using ThreadPtr          = std::unique_ptr<std::thread>;
  using TransactionList    = std::vector<chain::Transaction>;

  static constexpr char const *LOGGING_NAME = "TransactionProcessor";

  // Construction / Destruction
  TransactionProcessor(StorageUnitInterface &storage, miner::MinerInterface &miner,
                       std::size_t num_threads);
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
  void AddTransaction(MutableTransaction const &mtx);
  void AddTransaction(MutableTransaction &&mtx);
  /// @}

  // Operators
  TransactionProcessor &operator=(TransactionProcessor const &) = delete;
  TransactionProcessor &operator=(TransactionProcessor &&) = delete;

protected:
  /// @name Unverified Transaction Sink
  /// @{
  void OnTransaction(chain::UnverifiedTransaction const &tx) override;
  /// @}

  /// @name Transaction Handlers
  /// @{
  void OnTransaction(chain::VerifiedTransaction const &tx) override;
  void OnTransactions(TransactionList const &txs) override;
  /// @}

private:
  StorageUnitInterface & storage_;
  miner::MinerInterface &miner_;
  TransactionVerifier    verifier_;
  ThreadPtr              poll_new_tx_thread_;
  std::atomic_bool       running_;

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
inline void TransactionProcessor::AddTransaction(MutableTransaction const &mtx)
{
  verifier_.AddTransaction(mtx);
}

/**
 * Add a single transaction to the processor
 *
 * @param tx The reference to the new transaction to be processed
 */
inline void TransactionProcessor::AddTransaction(MutableTransaction &&mtx)
{
  verifier_.AddTransaction(std::move(mtx));
}

}  // namespace ledger
}  // namespace fetch
