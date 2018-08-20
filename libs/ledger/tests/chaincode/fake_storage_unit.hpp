#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class FakeStorageUnit : public fetch::ledger::StorageUnitInterface
{
public:
  using transaction_store_type =
      std::unordered_map<fetch::byte_array::ConstByteArray, fetch::chain::Transaction,
                         fetch::crypto::CallableFNV>;
  using state_store_type =
      std::unordered_map<fetch::byte_array::ConstByteArray, fetch::byte_array::ConstByteArray,
                         fetch::crypto::CallableFNV>;
  using state_archive_type = std::unordered_map<bookmark_type, state_store_type>;
  using lock_store_type =
      std::unordered_set<fetch::byte_array::ConstByteArray, fetch::crypto::CallableFNV>;
  using mutex_type      = std::mutex;
  using lock_guard_type = std::lock_guard<mutex_type>;

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

  bool Lock(ResourceAddress const &key) override
  {
    lock_guard_type lock(mutex_);
    bool            success = false;

    bool const already_locked = locks_.find(key.id()) != locks_.end();
    if (!already_locked)
    {
      locks_.insert(key.id());
      success = true;
    }

    return success;
  }

  bool Unlock(ResourceAddress const &key) override
  {
    lock_guard_type lock(mutex_);
    bool            success = false;

    bool const already_locked = locks_.find(key.id()) != locks_.end();
    if (already_locked)
    {
      locks_.erase(key.id());
      success = true;
    }

    return success;
  }

  void AddTransaction(fetch::chain::Transaction const &tx) override
  {
    lock_guard_type lock(mutex_);
    transactions_[tx.digest()] = tx;
  }

  bool GetTransaction(fetch::byte_array::ConstByteArray const &digest,
                      fetch::chain::Transaction &              tx) override
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

  hash_type Hash() override
  {
    lock_guard_type       lock(mutex_);
    fetch::crypto::SHA256 hasher{};

    std::vector<fetch::byte_array::ConstByteArray> keys;
    for (auto const &elem : state_)
    {
      keys.emplace_back(elem.first);
    }

    std::sort(keys.begin(), keys.end());

    hasher.Reset();
    for (auto const &key : keys)
    {
      hasher.Update(key);
      hasher.Update(state_[key]);
    }
    hasher.Final();

    return hasher.digest();
  }

  void Commit(bookmark_type const &bookmark) override
  {
    lock_guard_type lock(mutex_);
    state_archive_[bookmark] = state_;
  }

  void Revert(bookmark_type const &bookmark) override
  {
    lock_guard_type lock(mutex_);
    auto            it = state_archive_.find(bookmark);
    if (it != state_archive_.end())
    {
      state_ = it->second;
    }
    else
    {
      fetch::logger.Info("Reverting to clean state: ", bookmark);

      state_.clear();
    }
  }

private:
  mutex_type             mutex_;
  transaction_store_type transactions_;
  state_store_type       state_;
  lock_store_type        locks_;
  state_archive_type     state_archive_;
};
