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

#include <cstddef>
#include <memory>
#include <thread>

namespace fetch {
namespace ledger {
namespace v2 {
class Transaction;
}

class TransactionSink;

class TransactionVerifier
{
public:
  static constexpr char const *LOGGING_NAME = "TxVerifier";

  using TransactionPtr = std::shared_ptr<v2::Transaction>;

  // Construction / Destruction
  explicit TransactionVerifier(TransactionSink &sink, std::size_t verifying_threads,
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
  void AddTransaction(TransactionPtr const &tx);
  void AddTransaction(TransactionPtr &&tx);
  /// @}

  // Operators
  TransactionVerifier &operator=(TransactionVerifier const &) = delete;
  TransactionVerifier &operator=(TransactionVerifier &&) = delete;

private:
  static constexpr std::size_t QUEUE_SIZE = 1u << 16u;  // 65K

  using Flag            = std::atomic<bool>;
  using VerifiedQueue   = core::MPSCQueue<TransactionPtr, QUEUE_SIZE>;
  using UnverifiedQueue = core::MPMCQueue<TransactionPtr, QUEUE_SIZE>;
  using ThreadPtr       = std::unique_ptr<std::thread>;
  using Threads         = std::vector<ThreadPtr>;
  using Sink            = TransactionSink;

  void Verifier();
  void Dispatcher();

  std::size_t const verifying_threads_;
  std::string const name_;
  Sink &            sink_;
  Flag              active_{true};
  Threads           threads_;
  VerifiedQueue     verified_queue_;
  UnverifiedQueue   unverified_queue_;
};

inline void TransactionVerifier::AddTransaction(TransactionPtr const &tx)
{
  unverified_queue_.Push(tx);
}

inline void TransactionVerifier::AddTransaction(TransactionPtr &&tx)
{
  unverified_queue_.Push(std::move(tx));
}

}  // namespace ledger
}  // namespace fetch
