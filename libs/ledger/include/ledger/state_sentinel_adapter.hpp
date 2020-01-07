#pragma once
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

#include "core/bitvector.hpp"
#include "ledger/state_adapter.hpp"
#include "vectorise/platform.hpp"

#include <cstdint>
#include <string>

namespace fetch {
namespace ledger {

/**
 * Read / Write interface between the VM IO interface the and main ledger state database. Will
 * actively check to ensure reads and writes occur on permissible resources.
 */
class StateSentinelAdapter : public StateAdapter
{
public:
  using ConstByteArray = byte_array::ConstByteArray;

  // Construction / Destruction
  StateSentinelAdapter(StorageInterface &storage, ConstByteArray scope, BitVector const &shards);
  ~StateSentinelAdapter() override;

  /// @name IO Observer Interface
  /// @{
  Status Read(std::string const &key, void *data, uint64_t &size) override;
  Status Write(std::string const &key, void const *data, uint64_t size) override;
  Status Exists(std::string const &key) override;
  /// @}

  /// @name Counter Access
  /// @{
  uint64_t num_lookups() const;
  uint64_t num_bytes_read() const;
  uint64_t num_bytes_written() const;
  /// @}

private:
  bool IsAllowedResource(std::string const &key) const;

  /// @name Shard Limits
  /// @{
  BitVector const shards_;
  /// @}

  /// @name Counters
  /// @{
  uint64_t lookups_{0};
  uint64_t bytes_read_{0};
  uint64_t bytes_written_{0};
  /// @}
};

}  // namespace ledger
}  // namespace fetch
