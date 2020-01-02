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

#include "chain/constants.hpp"
#include "chain/transaction.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace ledger {

class FakeStorageUnit final : public StorageUnitInterface
{
public:
  using Transaction = fetch::chain::Transaction;
  using Digest      = fetch::Digest;
  using DigestSet   = fetch::DigestSet;
  using ResourceID  = fetch::storage::ResourceID;

  /// @name State Interface
  /// @{
  Document Get(ResourceAddress const &key) const override;
  Document GetOrCreate(ResourceAddress const &key) override;
  void     Set(ResourceAddress const &key, StateValue const &value) override;
  bool     Lock(ShardIndex index) override;
  bool     Unlock(ShardIndex index) override;
  /// @}

  /// @name Transaction Interface
  /// @{
  void AddTransaction(Transaction const &tx) override;
  bool GetTransaction(Digest const &digest, Transaction &tx) override;
  bool HasTransaction(Digest const &digest) override;
  void IssueCallForMissingTxs(DigestSet const &digests) override;
  /// @}

  /// @name Transaction History Poll
  /// @{
  TxLayouts PollRecentTx(uint32_t /*unused*/) override;
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
  Hash EmulateCommit(Hash const &commit_hash, uint64_t index);

  // Required to emulate the state being changed
  void SetCurrentHash(Hash const &hash);

  // Use the current state to set the hash
  void UpdateHash();

  void Reset() override;

private:
  using TransactionStore = std::map<Digest, Transaction>;
  using State            = std::map<ResourceAddress, StateValue>;
  using StatePtr         = std::shared_ptr<State>;
  using StateHistory     = std::unordered_map<Hash, StatePtr>;
  using StateHashStack   = std::vector<Hash>;

  mutable std::recursive_mutex lock_;
  TransactionStore             transaction_store_{};
  StatePtr                     state_{std::make_shared<State>()};
  StateHistory                 state_history_{};
  StateHashStack               state_history_stack_{fetch::chain::GetGenesisDigest()};
  Hash                         current_hash_{fetch::chain::GetGenesisMerkleRoot()};
};

}  // namespace ledger
}  // namespace fetch
