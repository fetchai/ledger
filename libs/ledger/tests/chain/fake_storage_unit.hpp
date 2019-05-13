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
#include "ledger/chain/v2/transaction.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

class FakeStorageUnit : public fetch::ledger::StorageUnitInterface
{
public:
  using Transaction = fetch::ledger::v2::Transaction;
  using Digest      = fetch::ledger::v2::Digest;
  using DigestSet   = fetch::ledger::v2::DigestSet;

  /// @name State Interface
  /// @{
  Document Get(ResourceAddress const &key) override;
  Document GetOrCreate(ResourceAddress const &key) override;
  void     Set(ResourceAddress const &key, StateValue const &value) override;
  bool     Lock(ShardIndex shard) override;
  bool     Unlock(ShardIndex shard) override;
  /// @}

  /// @name Transaction Interface
  /// @{
  void AddTransaction(Transaction const &tx) override;
  bool GetTransaction(Digest const &digest, Transaction &tx) override;
  bool HasTransaction(Digest const &digest) override;
  void IssueCallForMissingTxs(DigestSet const &tx_set) override;
  /// @}

  /// @name Transaction History Poll
  /// @{
  TxLayouts PollRecentTx(uint32_t) override;
  /// @}

  /// @name Revertible Document Store Interface
  /// @{
  Hash CurrentHash() override;
  Hash LastCommitHash() override;
  bool RevertToHash(Hash const &hash, uint64_t index) override;
  Hash Commit(uint64_t index) override;
  bool HashExists(Hash const &hash, uint64_t index) override;
  /// @}

  // Useful for test to force the hash
  Hash EmulateCommit(Hash const &hash, uint64_t index);

  // Required to emulate the state being changed
  void SetCurrentHash(Hash const &hash);

  // Use the current state to set the hash
  void UpdateHash();

private:
  using TransactionStore = std::map<Digest, Transaction>;
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
