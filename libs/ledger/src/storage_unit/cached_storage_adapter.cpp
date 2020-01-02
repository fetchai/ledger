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

#include "ledger/storage_unit/cached_storage_adapter.hpp"

#include <cassert>

namespace fetch {
namespace ledger {

/**
 * Construct the Cache Adapter
 *
 * @param storage The reference to the underlying storage engine
 */
CachedStorageAdapter::CachedStorageAdapter(StorageInterface &storage)
  : storage_{storage}
{}

/**
 * Destruct the Cache Adapter, this might cause a flush to occur
 */
CachedStorageAdapter::~CachedStorageAdapter() = default;

/**
 * Trigger a flush of the cached entries to the storage engine
 */
void CachedStorageAdapter::Flush()
{
  cache_.ApplyVoid([this](auto &cache) {
    for (auto &entry : cache)
    {
      if (!entry.second.flushed)
      {
        // set the value on the storage engine
        storage_.Set(entry.first, entry.second.value);

        // signal the entry as flushed
        entry.second.flushed = true;
      }
    }
  });
}

/**
 * Clear any cached values
 */
void CachedStorageAdapter::Clear()
{
  cache_.ApplyVoid([](auto &cache) { cache.clear(); });
}

/**
 * Get a resource from the storage engine or cache
 *
 * @param key The key to be accessed
 * @return The document containing the result
 */
CachedStorageAdapter::Document CachedStorageAdapter::Get(ResourceAddress const &key) const
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
    result = storage_.Get(key);

    if (!result.failed)
    {
      // update the result
      AddCacheEntry(key, result.document);
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
    result = storage_.GetOrCreate(key);

    if (!result.failed)
    {
      // update the result
      AddCacheEntry(key, result.document);
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
 * @param index The shard index to be locked
 * @return true if successful, otherwise false
 */
bool CachedStorageAdapter::Lock(ShardIndex index)
{
  // proxy this call directly to the underlying storage engine
  return storage_.Lock(index);
}

/**
 * Unlock a resource on the storage engine
 *
 * @param index The shard index to be unlocked
 * @return true if successful, otherwise false
 */
bool CachedStorageAdapter::Unlock(ShardIndex index)
{
  // proxy this call directly to the underlying storage engine
  return storage_.Unlock(index);
}

/**
 * Add an entry to the cache
 *
 * @param address The address of the resource being stored
 * @param value The value being stored
 */
void CachedStorageAdapter::AddCacheEntry(ResourceAddress const &address,
                                         StateValue const &     value) const
{
  // update the cache and signal that a flush is required
  cache_.ApplyVoid([&address, &value](auto &cache) { cache[address] = CacheEntry{value}; });
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
  return cache_.Apply([&address](auto const &cache) -> CachedStorageAdapter::StateValue {
    StateValue value{};

    // ensure the key exists
    auto it = cache.find(address);
    detailed_assert(it != cache.end());
    value = it->second.value;

    return value;
  });
}

/**
 * Determine if a resource is being stored in the cache
 *
 * @param address The resource address to be checked
 * @return true if being stored, otherwise false
 */
bool CachedStorageAdapter::HasCacheEntry(ResourceAddress const &address) const
{
  return cache_.Apply(
      [&address](auto const &cache) -> bool { return cache.find(address) != cache.end(); });
}

/**
 * Reset the database
 */
void CachedStorageAdapter::Reset()
{
  return storage_.Reset();
}

}  // namespace ledger
}  // namespace fetch
