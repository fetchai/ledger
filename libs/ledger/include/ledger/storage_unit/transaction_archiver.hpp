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
#include "core/digest.hpp"
#include "core/state_machine.hpp"

namespace fetch {
namespace ledger {

class TransactionPoolInterface;
class TransactionStoreInterface;

/**
 * The transaction archiver manages transactions between a pool and a store. Once a transaction
 * has been confirmed it will be placed in a queue which will result in the transaction being
 * committed to persistent storage.
 *
 *                       ┌─────────────┐               ┌─────────────┐
 *                       │ Transaction │               │ Transaction │
 *                       │    Pool     │               │    Store    │
 *                       └─────────────┘               └─────────────┘
 *                              │                             ▲
 *                              │                             │
 *                              │       ┌─────────────┐       │
 *                              │       │ Transaction │       │
 *                              └──────▶│  Archiver   │───────┘
 *                                      └─────────────┘
 *
 */
class TransactionArchiver
{
public:
  enum class State
  {
    COLLECTING,
    FLUSHING
  };

  using StateMachine    = core::StateMachine<State>;
  using StateMachinePtr = std::shared_ptr<StateMachine>;

  TransactionArchiver(TransactionPoolInterface &pool, TransactionStoreInterface &archive);
  ~TransactionArchiver() = default;

  void Confirm(Digest const &digest);

  StateMachinePtr const &GetStateMachine() const;

private:
  static const std::size_t BATCH_SIZE = 100;

  using ConfirmationQueue = core::MPMCQueue<Digest, 1u << 15u>;
  using Digests           = std::vector<Digest>;

  State OnCollecting();
  State OnFlushing();

  TransactionPoolInterface & pool_;
  TransactionStoreInterface &archive_;
  ConfirmationQueue          confirmation_queue_;

  // State Machine state
  StateMachinePtr state_machine_;
  Digests         digests_;
};

char const *ToString(TransactionArchiver::State state);

}  // namespace ledger
}  // namespace fetch
