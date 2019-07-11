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

#include "in_memory_storage.hpp"

using Document  = InMemoryStorageUnit::Document;
using Keys      = InMemoryStorageUnit::Keys;
using TxLayouts = InMemoryStorageUnit::TxLayouts;
using Hash      = InMemoryStorageUnit::Hash;

Document InMemoryStorageUnit::Get(ResourceAddress const &key)
{
  Document ret;

  auto it = state_->find(key.id());
  if (it != state_->end())
  {
    ret.document = it->second;
  }
  else
  {
    ret.failed = true;
  }

  return ret;
}

Document InMemoryStorageUnit::GetOrCreate(ResourceAddress const &key)
{
  Document ret;

  auto it = state_->find(key.id());
  if (it != state_->end())
  {
    ret.document = it->second;
  }
  else
  {
    // create a new empty instance in the database
    (*state_)[key.id()] = ret.document;

    ret.was_created = true;
    ret.failed = false;
  }

  return ret;
}

void InMemoryStorageUnit::Set(ResourceAddress const &key, StateValue const &value)
{
  (*state_)[key.id()] = value;
}

bool InMemoryStorageUnit::Lock(ShardIndex shard)
{
  bool success{false};

  auto it = locks_.find(shard);
  if (it == locks_.end())
  {
    locks_.insert(shard);
    success = true;
  }

  return success;
}

bool InMemoryStorageUnit::Unlock(ShardIndex shard)
{
  bool success{false};

  auto it = locks_.find(shard);
  if (it != locks_.end())
  {
    locks_.erase(it);
    success = true;
  }

  return success;
}

Keys InMemoryStorageUnit::KeyDump() const
{
  throw std::runtime_error("Not implemented by design");
}

void InMemoryStorageUnit::Reset()
{
  throw std::runtime_error("Not implemented by design");
}

void InMemoryStorageUnit::AddTransaction(Transaction const &tx)
{
  tx_store_[tx.digest()] = std::make_shared<Transaction>(tx);
}

bool InMemoryStorageUnit::GetTransaction(Digest const &digest, Transaction &tx)
{
  bool success{false};

  auto it = tx_store_.find(digest);
  if (it != tx_store_.end())
  {
    tx      = *(it->second);
    success = true;
  }

  return success;
}

bool InMemoryStorageUnit::HasTransaction(Digest const &digest)
{
  return tx_store_.find(digest) != tx_store_.end();
}

void InMemoryStorageUnit::IssueCallForMissingTxs(DigestSet const &)
{
  throw std::runtime_error("Not implemented by design");
}

TxLayouts InMemoryStorageUnit::PollRecentTx(uint32_t)
{
  throw std::runtime_error("Not implemented by design");
}

Hash InMemoryStorageUnit::CurrentHash()
{
  throw std::runtime_error("Not implemented by design");
}

Hash InMemoryStorageUnit::LastCommitHash()
{
  throw std::runtime_error("Not implemented by design");
}

bool InMemoryStorageUnit::RevertToHash(Hash const &, uint64_t)
{
  throw std::runtime_error("Not implemented by design");
}

Hash InMemoryStorageUnit::Commit(uint64_t)
{
  throw std::runtime_error("Not implemented by design");
}

bool InMemoryStorageUnit::HashExists(Hash const &, uint64_t)
{
  throw std::runtime_error("Not implemented by design");
}
