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
#include "core/synchronisation/protected.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <string>
#include <unordered_map>
#include <utility>

namespace fetch {
namespace ledger {

/**
 * Designed for temporary caching of values to reduce hits to the underlying storage engine.
 *
 * Initially intended in conjunction with the smart contract engine
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
  Document Get(ResourceAddress const &key) const override;
  Document GetOrCreate(ResourceAddress const &key) override;
  void     Set(ResourceAddress const &key, StateValue const &value) override;
  bool     Lock(ShardIndex index) override;
  bool     Unlock(ShardIndex index) override;
  void     Reset() override;
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

  /// @name Cache Helpers
  /// @{
  void       AddCacheEntry(ResourceAddress const &address, StateValue const &value) const;
  StateValue GetCacheEntry(ResourceAddress const &address) const;
  bool       HasCacheEntry(ResourceAddress const &address) const;
  /// @}

  StorageInterface &storage_;  ///< The reference to the underlying storage engine

  /// @name Cache Data
  /// @{
  mutable Protected<Cache> cache_{};  ///< The local cache
  /// @}
};

}  // namespace ledger
}  // namespace fetch
