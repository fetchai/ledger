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

#include "ledger/storage_unit/cached_storage_adapter.hpp"

namespace fetch {
namespace ledger {

/**
 * Construct the Cache Adpater
 *
 * @param storage The reference to the underlying storage engine
 */
CachedStorageAdapter::CachedStorageAdapter(StorageInterface &storage)
  : storage_{storage}
{}

/**
 * Destruct the Cache Adapter, this might cause a flush to occur
 */
CachedStorageAdapter::~CachedStorageAdapter()
{}

/**
 * Trigger a flush of the cached entries to the storage engine
 */
void CachedStorageAdapter::Flush()
{
  if (flush_required_)
  {
    for (auto &entry : cache_)
    {
      if (!entry.second.flushed)
      {
        // set the value on the storage engine
        storage_.Set(entry.first, entry.second.value);

        // signal the entry as flushed
        entry.second.flushed = true;
      }
    }

    // reset the top level flush flag
    flush_required_ = false;
  }
}

/**
 * Get a resource from the storage engine or cache
 *
 * @param key The key to be accessed
 * @return The document containing the result
 */
CachedStorageAdapter::Document CachedStorageAdapter::Get(ResourceAddress const &key)
{
  Document result;

  // check to see if the value is in the cache
  if (HasCacheEntry(key))
  {
    // retrieve the document directly from the cache
    result.document = GetCacheEntry(key);
  }
  else
  {
    // not in the cache need to retrieve
    auto const storage_result = storage_.Get(key);

    if (!result.failed)
    {
      // update the result
      AddCacheEntry(key, storage_result.document);

      // update the result
      result = storage_result;
    }
  }

  return result;
}

/**
 * Get or Create a resource in the storage engine
 *
 * @param key The key to be accessed
 * @return The document containing the result
 */
CachedStorageAdapter::Document CachedStorageAdapter::GetOrCreate(ResourceAddress const &key)
{
  Document result;

  // check to see if the value is in the cache
  if (HasCacheEntry(key))
  {
    // retrieve the document directly from the cache
    result.document = GetCacheEntry(key);
  }
  else
  {
    // not in the cache need to retrieve
    auto const storage_result = storage_.GetOrCreate(key);

    if (!result.failed)
    {
      // update the result
      AddCacheEntry(key, storage_result.document);

      // update the result
      result = storage_result;
    }
  }

  return result;
}

/**
 * Set a value to storage engine
 *
 * @param key The key of the value
 * @param value The value being set
 */
void CachedStorageAdapter::Set(ResourceAddress const &key, StateValue const &value)
{
  // set the value directly into the cache
  AddCacheEntry(key, value);
}

/**
 * Lock a resource on the storage engine
 *
 * @param key The key of resource to be locked
 * @return true if successful, otherwise false
 */
bool CachedStorageAdapter::Lock(ResourceAddress const &key)
{
  // proxy this call directly to the underlying storage engine
  return storage_.Lock(key);
}

/**
 * Unlock a resource on the storage engine
 *
 * @param key The key of the resource to be unlocked
 * @return true if successful, otherwise false
 */
bool CachedStorageAdapter::Unlock(ResourceAddress const &key)
{
  // proxy this call directly to the underlying storage engine
  return storage_.Unlock(key);
}

/**
 * Add an entry to the cache
 *
 * @param address The address of the resource being stored
 * @param value The value being stored
 */
void CachedStorageAdapter::AddCacheEntry(ResourceAddress const &address, StateValue const &value)
{
  // update the cache and signal that a flush is required
  cache_[address] = CacheEntry{value};
  flush_required_ = true;
}

/**
 * Get a value being stored in the cache
 *
 * @param address The address of the resource being stored
 * @return The value being stored
 */
CachedStorageAdapter::StateValue CachedStorageAdapter::GetCacheEntry(
    ResourceAddress const &address) const
{
  StateValue value{};

  // ensure the key exists
  auto it = cache_.find(address);
  if (it != cache_.end())
  {
    value = it->second.value;
  }
  else
  {
    // sanity check
    assert(false);
  }

  return value;
}

/**
 * Determine if a resource is being stored in the cache
 *
 * @param address The resource address to be checked
 * @return true if being stored, otherwise false
 */
bool CachedStorageAdapter::HasCacheEntry(ResourceAddress const &address) const
{
  return cache_.find(address) != cache_.end();
}

}  // namespace ledger
}  // namespace fetch
