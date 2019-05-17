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

#include "fake_storage_unit.hpp"

#include "core/macros.hpp"
#include "core/byte_array/byte_array.hpp"
#include "crypto/sha256.hpp"

#include <algorithm>

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

bool FakeStorageUnit::Lock(ShardIndex index)
{
  FETCH_UNUSED(index);
  return true;
}

bool FakeStorageUnit::Unlock(ShardIndex index)
{
  FETCH_UNUSED(index);
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

void FakeStorageUnit::IssueCallForMissingTxs(DigestSet const &digests)
{
  FETCH_UNUSED(digests);
}

FakeStorageUnit::TxLayouts FakeStorageUnit::PollRecentTx(uint32_t)
{
  return {};
}

// We need to be able to set the 'hash' since it isn't calculated from any state changes
void FakeStorageUnit::SetCurrentHash(FakeStorageUnit::Hash const &hash)
{
  current_hash_ = hash;
}

// We need to be able to set the 'hash' from the current state
void FakeStorageUnit::UpdateHash()
{
  fetch::crypto::SHA256 sha256{};

  for (auto const &kv : *state_)
  {
    auto state_value = kv.second;
    sha256.Update(state_value);
  }

  current_hash_ = sha256.Final();
}

FakeStorageUnit::Hash FakeStorageUnit::CurrentHash()
{
  return current_hash_;
}

FakeStorageUnit::Hash FakeStorageUnit::LastCommitHash()
{
  assert(!state_history_stack_.empty());

  return state_history_stack_.back();
}

bool FakeStorageUnit::RevertToHash(Hash const &hash, uint64_t index)
{
  FETCH_UNUSED(index);

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

FakeStorageUnit::Hash FakeStorageUnit::Commit(uint64_t index)
{
  // calculate the new "hash" for the state
  Hash commit_hash = current_hash_;

  return EmulateCommit(commit_hash, index);
}

bool FakeStorageUnit::HashExists(Hash const &hash, uint64_t /*index*/)
{
  auto const it  = std::find(state_history_stack_.begin(), state_history_stack_.end(), hash);
  bool       res = (state_history_stack_.end() != it);
  return res;
}

FakeStorageUnit::Hash FakeStorageUnit::EmulateCommit(Hash const &commit_hash, uint64_t index)
{
  if (state_history_.find(commit_hash) != state_history_.end() && index != 0)
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
