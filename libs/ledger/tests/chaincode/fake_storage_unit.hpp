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

#include "chain/transaction.hpp"
#include "core/mutex.hpp"
#include "crypto/fnv.hpp"
#include "crypto/sha256.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class FakeStorageUnit final : public fetch::ledger::StorageUnitInterface
{
public:
  using TransactionStoreType =
      std::unordered_map<fetch::byte_array::ConstByteArray, fetch::chain::Transaction>;
  using StateStoreType =
      std::unordered_map<fetch::byte_array::ConstByteArray, fetch::byte_array::ConstByteArray>;
  using LockStoreType = std::unordered_set<ShardIndex>;
  using MutexType     = std::mutex;
  using HashType      = fetch::byte_array::ConstByteArray;
  using ResourceID    = fetch::storage::ResourceID;

  Document GetOrCreate(ResourceAddress const &key) override
  {
    FETCH_LOCK(mutex_);

    Document doc;

    auto it = state_.find(key.id());
    if (it != state_.end())
    {
      doc.document = it->second;
    }
    else
    {
      doc.was_created = true;
    }

    return doc;
  }

  Document Get(ResourceAddress const &key) override
  {
    FETCH_LOCK(mutex_);
    Document doc;

    auto it = state_.find(key.id());
    if (it != state_.end())
    {
      doc.document = it->second;
    }
    else
    {
      doc.failed = true;
    }

    return doc;
  }

  void Set(ResourceAddress const &key, StateValue const &value) override
  {
    FETCH_LOCK(mutex_);

    state_[key.id()] = value;
  }

  bool Lock(ShardIndex shard) override
  {
    FETCH_LOCK(mutex_);

    bool success = false;

    bool const already_locked = locks_.find(shard) != locks_.end();
    if (!already_locked)
    {
      locks_.insert(shard);
      success = true;
    }

    return success;
  }

  bool Unlock(ShardIndex shard) override
  {
    FETCH_LOCK(mutex_);

    bool success = false;

    bool const already_locked = locks_.find(shard) != locks_.end();
    if (already_locked)
    {
      locks_.erase(shard);
      success = true;
    }

    return success;
  }

  void AddTransaction(fetch::chain::Transaction const &tx) override
  {
    FETCH_LOCK(mutex_);
    transactions_[tx.digest()] = tx;
  }

  bool GetTransaction(fetch::byte_array::ConstByteArray const &digest,
                      fetch::chain::Transaction &              tx) override
  {
    FETCH_LOCK(mutex_);
    bool success = false;

    auto it = transactions_.find(digest);
    if (it != transactions_.end())
    {
      tx      = it->second;
      success = true;
    }

    return success;
  }

  bool HasTransaction(ConstByteArray const &digest) override
  {
    FETCH_LOCK(mutex_);
    return transactions_.find(digest) != transactions_.end();
  }

  void IssueCallForMissingTxs(fetch::DigestSet const & /*tx_set*/) override
  {}

  Hash CurrentHash() override
  {
    return "";
  };
  Hash LastCommitHash() override
  {
    return "";
  };
  bool RevertToHash(Hash const & /*hash*/, uint64_t /*index*/) override
  {
    return true;
  };
  Hash Commit(uint64_t /*index*/) override
  {
    return "";
  };
  bool HashExists(Hash const & /*hash*/, uint64_t /*index*/) override
  {
    return true;
  };

  // Does nothing
  TxLayouts PollRecentTx(uint32_t /*unused*/) override
  {
    return {};
  }

  void Reset() override
  {
    state_.clear();
    transactions_.clear();
  }

private:
  MutexType            mutex_;
  TransactionStoreType transactions_;
  StateStoreType       state_;
  LockStoreType        locks_;
};
