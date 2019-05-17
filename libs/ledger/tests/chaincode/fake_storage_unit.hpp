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

#include "crypto/fnv.hpp"
#include "crypto/sha256.hpp"
#include "ledger/chain/v2/transaction.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class FakeStorageUnit final : public fetch::ledger::StorageUnitInterface
{
public:
  using transaction_store_type =
      std::unordered_map<fetch::byte_array::ConstByteArray, fetch::ledger::v2::Transaction>;
  using state_store_type =
      std::unordered_map<fetch::byte_array::ConstByteArray, fetch::byte_array::ConstByteArray>;
  /*using state_archive_type = std::unordered_map<bookmark_type, state_store_type>; */
  using lock_store_type = std::unordered_set<ShardIndex>;
  using mutex_type      = std::mutex;
  using lock_guard_type = std::lock_guard<mutex_type>;
  using hash_type       = fetch::byte_array::ConstByteArray;

  static constexpr char const *LOGGING_NAME = "FakeStorageUnit";

  Document GetOrCreate(ResourceAddress const &key) override
  {
    lock_guard_type lock(mutex_);
    Document        doc;

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
    lock_guard_type lock(mutex_);
    Document        doc;

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
    lock_guard_type lock(mutex_);
    state_[key.id()] = value;
  }

  bool Lock(ShardIndex shard) override
  {
    lock_guard_type lock(mutex_);
    bool            success = false;

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
    lock_guard_type lock(mutex_);
    bool            success = false;

    bool const already_locked = locks_.find(shard) != locks_.end();
    if (already_locked)
    {
      locks_.erase(shard);
      success = true;
    }

    return success;
  }

  void AddTransaction(fetch::ledger::v2::Transaction const &tx) override
  {
    lock_guard_type lock(mutex_);
    transactions_[tx.digest()] = tx;
  }

  bool GetTransaction(fetch::byte_array::ConstByteArray const &digest,
                      fetch::ledger::v2::Transaction &         tx) override
  {
    lock_guard_type lock(mutex_);
    bool            success = false;

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
    lock_guard_type lock(mutex_);
    return transactions_.find(digest) != transactions_.end();
  }

  void IssueCallForMissingTxs(fetch::ledger::v2::DigestSet const &) override
  {}

  Hash CurrentHash() override
  {
    return "";
  };
  Hash LastCommitHash() override
  {
    return "";
  };
  bool RevertToHash(Hash const &, uint64_t) override
  {
    return true;
  };
  Hash Commit(uint64_t) override
  {
    return "";
  };
  bool HashExists(Hash const &, uint64_t) override
  {
    return true;
  };

  // Does nothing
  TxLayouts PollRecentTx(uint32_t) override
  {
    return {};
  }

private:
  mutex_type             mutex_;
  transaction_store_type transactions_;
  state_store_type       state_;
  lock_store_type        locks_;
  /* state_archive_type     state_archive_; */
};
