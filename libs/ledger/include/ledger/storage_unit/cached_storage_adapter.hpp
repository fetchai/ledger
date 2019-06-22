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

#include "core/mutex.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <string>
#include <unordered_map>
#include <utility>

namespace fetch {
namespace ledger {

/**
 * Designed for temporary caching of values to reduce hits to the underlying storage engine.
 *
 * Initially intended in conjuction with the smart contract engine
 */
class CachedStorageAdapter : public StorageInterface
{
public:
  // Construction / Destruction
  explicit CachedStorageAdapter(StorageInterface &storage);
  ~CachedStorageAdapter() override;

  void Flush();
  void Clear();

  /// @name State Interface
  /// @{
  Document Get(ResourceAddress const &key) override;
  Document GetOrCreate(ResourceAddress const &key) override;
  void     Set(ResourceAddress const &key, StateValue const &value) override;
  bool     Lock(ShardIndex index) override;
  bool     Unlock(ShardIndex index) override;
  Keys     KeyDump() const override;
  /// @}

private:
  struct CacheEntry
  {
    StateValue value{};
    bool       flushed{false};

    CacheEntry() = default;
    explicit CacheEntry(StateValue v)
      : value{std::move(v)}
    {}
  };

  using Cache = std::unordered_map<ResourceAddress, CacheEntry>;
  using Mutex = mutex::Mutex;

  /// @name Cache Helpers
  /// @{
  void       AddCacheEntry(ResourceAddress const &address, StateValue const &value);
  StateValue GetCacheEntry(ResourceAddress const &address) const;
  bool       HasCacheEntry(ResourceAddress const &address) const;
  /// @}

  StorageInterface &storage_;  ///< The reference to the underlying storage engine

  /// @name Cache Data
  /// @{
  mutable Mutex lock_{__LINE__, __FILE__};
  Cache         cache_{};                ///< The local cache
  bool          flush_required_{false};  ///< Top level cache flush flag
  /// @}
};

}  // namespace ledger
}  // namespace fetch
