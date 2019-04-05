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

#include "core/byte_array/byte_array.hpp"

#include "fake_storage_unit.hpp"

#include <algorithm>

using fetch::byte_array::ByteArray;

FakeStorageUnit::Document FakeStorageUnit::Get(ResourceAddress const &key)
{
  Document doc;

  auto it = state_->find(key);
  if (it != state_->end())
  {
    doc.document = it->second;
  }
  else
  {
    doc.failed = true;
  }

  return doc;
}

FakeStorageUnit::Document FakeStorageUnit::GetOrCreate(ResourceAddress const &key)
{
  Document doc = Get(key);

  if (doc.failed)
  {
    // create a value
    (*state_)[key] = StateValue{};

    // flip the signals
    doc.failed      = false;
    doc.was_created = true;
  }

  return doc;
}

void FakeStorageUnit::Set(ResourceAddress const &key, StateValue const &value)
{
  (*state_)[key] = value;
}

bool FakeStorageUnit::Lock(ResourceAddress const &key)
{
  FETCH_UNUSED(key);
  return true;
}

bool FakeStorageUnit::Unlock(ResourceAddress const &key)
{
  FETCH_UNUSED(key);
  return true;
}

void FakeStorageUnit::AddTransaction(Transaction const &tx)
{
  transaction_store_[tx.digest()] = tx;
}

bool FakeStorageUnit::GetTransaction(ConstByteArray const &digest, Transaction &tx)
{
  bool success{false};

  auto it = transaction_store_.find(digest);
  if (it != transaction_store_.end())
  {
    tx      = it->second;
    success = true;
  }

  return success;
}

bool FakeStorageUnit::HasTransaction(ConstByteArray const &digest)
{
  return transaction_store_.find(digest) != transaction_store_.end();
}

FakeStorageUnit::TxSummaries FakeStorageUnit::PollRecentTx(uint32_t)
{
  return {};
}

FakeStorageUnit::Hash FakeStorageUnit::CurrentHash()
{
  return current_hash_;
}

FakeStorageUnit::Hash FakeStorageUnit::LastCommitHash()
{
  return current_hash_;
}

bool FakeStorageUnit::RevertToHash(Hash const &hash)
{
  bool success{false};

  // attempt to locate the hash in the current stack
  auto const it = std::find(state_history_stack_.rbegin(), state_history_stack_.rend(), hash);

  if (it != state_history_stack_.rend())
  {
    // emulate database by removing all other state hashes
    state_history_stack_.erase(it.base(), state_history_stack_.end());

    // sanity check
    if (state_history_.find(hash) == state_history_.end())
    {
      throw std::runtime_error("Syncronisation issue between map and stack");
    }

    // perform the reverting options
    current_hash_ = hash;
    state_        = std::make_shared<State>(*state_history_[hash]);
    success       = true;
  }

  return success;
}

FakeStorageUnit::Hash FakeStorageUnit::Commit()
{
  // calculate the new "hash" for the state
  Hash commit_hash = CreateHashFromCounter(++hash_counter_);

  return EmulateCommit(commit_hash);
}

bool FakeStorageUnit::HashExists(Hash const &hash)
{
  auto const it = std::find(state_history_stack_.begin(), state_history_stack_.end(), hash);
  return (state_history_stack_.end() != it);
}

FakeStorageUnit::Hash FakeStorageUnit::EmulateCommit(Hash const &commit_hash)
{
  if (state_history_.find(commit_hash) != state_history_.end())
  {
    throw std::runtime_error("Duplicate state hash request");
  }

  // create a copy of the state and mark the current hash
  state_history_[commit_hash] = std::make_shared<State>(*state_);
  current_hash_               = commit_hash;

  // emulate file based stack
  state_history_stack_.push_back(commit_hash);

  return commit_hash;
}

FakeStorageUnit::Hash FakeStorageUnit::CreateHashFromCounter(uint64_t count)
{
  static constexpr std::size_t HASH_LENGTH = 32;
  static_assert(HASH_LENGTH % sizeof(count) == 0, "");  // not 100% important

  auto const *const count_bytes = reinterpret_cast<uint8_t *>(&count);

  ByteArray hash;
  hash.Resize(HASH_LENGTH);

  for (std::size_t i = 0; i < sizeof(count); ++i)
  {
    for (std::size_t j = i; j < HASH_LENGTH; j += sizeof(count))
    {
      hash[j] = count_bytes[i];
    }
  }

  return {hash};
}