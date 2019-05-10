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

#include "ledger/chain/block.hpp"
#include "ledger/chain/constants.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

class FakeStorageUnit : public fetch::ledger::StorageUnitInterface
{
public:
  using Transaction = fetch::ledger::Transaction;
  using TxDigestSet = fetch::ledger::TxDigestSet;

  /// @name State Interface
  /// @{
  Document Get(ResourceAddress const &key);
  Document GetOrCreate(ResourceAddress const &key);
  void     Set(ResourceAddress const &key, StateValue const &value);
  bool     Lock(ResourceAddress const &key);
  bool     Unlock(ResourceAddress const &key);
  /// @}

  /// @name Transaction Interface
  /// @{
  void AddTransaction(Transaction const &tx);
  bool GetTransaction(ConstByteArray const &digest, Transaction &tx);
  bool HasTransaction(ConstByteArray const &digest);
  void IssueCallForMissingTxs(TxDigestSet const &tx_set);
  /// @}

  /// @name Transaction History Poll
  /// @{
  TxSummaries PollRecentTx(uint32_t);
  /// @}

  /// @name Revertible Document Store Interface
  /// @{
  Hash CurrentHash();
  Hash LastCommitHash();
  bool RevertToHash(Hash const &hash, uint64_t index);
  Hash Commit(uint64_t index);
  bool HashExists(Hash const &hash, uint64_t index);
  /// @}

  // Useful for test to force the hash
  Hash EmulateCommit(Hash const &hash, uint64_t index);

  // Required to emulate the state being changed
  void SetCurrentHash(Hash const &hash);

  // Use the current state to set the hash
  void UpdateHash();

private:
  using TransactionStore = std::map<Transaction::TxDigest, Transaction>;
  using State            = std::map<ResourceAddress, StateValue>;
  using StatePtr         = std::shared_ptr<State>;
  using StateHistory     = std::unordered_map<Hash, StatePtr>;
  using StateHashStack   = std::vector<Hash>;

  TransactionStore transaction_store_{};
  StatePtr         state_{std::make_shared<State>()};
  StateHistory     state_history_{};
  StateHashStack   state_history_stack_{fetch::ledger::GENESIS_MERKLE_ROOT};
  Hash             current_hash_{fetch::ledger::GENESIS_MERKLE_ROOT};
  Hash             last_commit_hash_{};
};
