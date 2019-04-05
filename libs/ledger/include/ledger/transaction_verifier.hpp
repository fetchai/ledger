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

#include "core/containers/queue.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/storage_unit/transaction_sinks.hpp"

#include <cstddef>
#include <thread>

namespace fetch {
namespace ledger {

class TransactionVerifier
{
public:
  static constexpr char const *LOGGING_NAME = "TxVerifier";

  // Construction / Destruction
  explicit TransactionVerifier(VerifiedTransactionSink &sink, std::size_t verifying_threads,
                               std::string name)
    : verifying_threads_(verifying_threads)
    , name_(std::move(name))
    , sink_(sink)
  {}
  TransactionVerifier(TransactionVerifier const &) = delete;
  TransactionVerifier(TransactionVerifier &&)      = delete;
  ~TransactionVerifier();

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
  TransactionVerifier &operator=(TransactionVerifier const &) = delete;
  TransactionVerifier &operator=(TransactionVerifier &&) = delete;

private:
  static constexpr std::size_t QUEUE_SIZE         = 1u << 16u;  // 65K
  static constexpr std::size_t DEFAULT_BATCH_SIZE = 1000;

  using Flag            = std::atomic<bool>;
  using VerifiedQueue   = core::MPSCQueue<VerifiedTransaction, QUEUE_SIZE>;
  using UnverifiedQueue = core::MPMCQueue<MutableTransaction, QUEUE_SIZE>;
  using ThreadPtr       = std::unique_ptr<std::thread>;
  using Threads         = std::vector<ThreadPtr>;
  using Sink            = VerifiedTransactionSink;

  void Verifier();
  void Dispatcher();

  std::size_t batch_size_{DEFAULT_BATCH_SIZE};
  std::size_t verifying_threads_;

  std::string const name_;
  std::string const logging_name_;
  Sink &            sink_;
  Flag              active_{true};
  Threads           threads_;
  VerifiedQueue     verified_queue_;
  UnverifiedQueue   unverified_queue_;
};

inline void TransactionVerifier::AddTransaction(MutableTransaction const &mtx)
{
  unverified_queue_.Push(mtx);
}

inline void TransactionVerifier::AddTransaction(MutableTransaction &&mtx)
{
  unverified_queue_.Push(std::move(mtx));
}

}  // namespace ledger
}  // namespace fetch
