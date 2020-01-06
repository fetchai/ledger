#pragma once
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

#include "ledger/dag/dag_interface.hpp"
#include "ledger/storage_unit/transaction_sinks.hpp"
#include "ledger/transaction_verifier.hpp"

#include <atomic>
#include <memory>
#include <thread>

namespace fetch {

namespace chain {

class Transaction;

}  // namespace chain

namespace ledger {

class StorageUnitInterface;
class BlockPackerInterface;
class TransactionStatusCache;

class TransactionProcessor : public TransactionSink
{
public:
  using DAGPtr           = std::shared_ptr<fetch::ledger::DAGInterface>;
  using TxStatusCachePtr = std::shared_ptr<TransactionStatusCache>;

  // Construction / Destruction
  TransactionProcessor(DAGPtr dag, StorageUnitInterface &storage, BlockPackerInterface &packer,
                       TxStatusCachePtr tx_status_cache, std::size_t num_threads);
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
  void AddTransaction(TransactionPtr const &tx);
  void AddTransaction(TransactionPtr &&tx);
  /// @}

  // Operators
  TransactionProcessor &operator=(TransactionProcessor const &) = delete;
  TransactionProcessor &operator=(TransactionProcessor &&) = delete;

protected:
  void OnTransaction(TransactionPtr const &tx) override;

private:
  using Flag      = std::atomic<bool>;
  using ThreadPtr = std::unique_ptr<std::thread>;

  DAGPtr                dag_;
  StorageUnitInterface &storage_;
  BlockPackerInterface &packer_;
  TxStatusCachePtr      status_cache_;
  TransactionVerifier   verifier_;
  ThreadPtr             poll_new_tx_thread_;
  Flag                  running_{false};

  void ThreadEntryPoint();
};

}  // namespace ledger
}  // namespace fetch
