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

#include "ledger/chain/transaction.hpp"
#include "ledger/chain/digest.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <unordered_map>
#include <unordered_set>
#include <memory>

class InMemoryStorageUnit : public fetch::ledger::StorageUnitInterface
{
public:
  using Transaction = fetch::ledger::Transaction;
  using Digest      = fetch::ledger::Digest;
  using DigestSet   = fetch::ledger::DigestSet;

  // Construction / Destruction
  InMemoryStorageUnit() = default;
  InMemoryStorageUnit(InMemoryStorageUnit const &) = delete;
  InMemoryStorageUnit(InMemoryStorageUnit &&) = delete;
  ~InMemoryStorageUnit() override = default;

  /// @name State Interface
  /// @{
  Document Get(ResourceAddress const &key) override;
  Document GetOrCreate(ResourceAddress const &key) override;
  void     Set(ResourceAddress const &key, StateValue const &value) override;
  bool     Lock(ShardIndex shard) override;
  bool     Unlock(ShardIndex shard) override;
  Keys     KeyDump() const override;
  void     Reset() override;
  /// @}

  /// @name Transaction Interface
  /// @{
  void AddTransaction(Transaction const &tx) override;
  bool GetTransaction(Digest const &digest, Transaction &tx) override;
  bool HasTransaction(Digest const &digest) override;
  void IssueCallForMissingTxs(DigestSet const &tx_set) override;
  /// @}

  TxLayouts PollRecentTx(uint32_t) override;

  /// @name Revertible Document Store Interface
  /// @{
  Hash CurrentHash() override;
  Hash LastCommitHash() override;
  bool RevertToHash(Hash const &hash, uint64_t index) override;
  Hash Commit(uint64_t index) override;
  bool HashExists(Hash const &hash, uint64_t index) override;
  /// @}

  // Operators
  InMemoryStorageUnit &operator=(InMemoryStorageUnit const &) = delete;
  InMemoryStorageUnit &operator=(InMemoryStorageUnit &&) = delete;

private:

  using TransactionPtr   = std::shared_ptr<Transaction>;
  using TransactionStore = std::unordered_map<Digest,TransactionPtr>;
  using StateSnapshot    = std::unordered_map<Digest,StateValue>;
  using StateSnapshotPtr = std::shared_ptr<StateSnapshot>;
  using StateSnapshots   = std::vector<StateSnapshotPtr>;
  using ShardLocks       = std::unordered_set<ShardIndex>;

  /// @name TX Information
  /// @{
  TransactionStore tx_store_{};
  /// @}

  /// @name State Information
  /// @{
  StateSnapshotPtr state_{std::make_shared<StateSnapshot>()};
  StateSnapshots   state_history_{};
  /// @}

  ShardLocks locks_;
};