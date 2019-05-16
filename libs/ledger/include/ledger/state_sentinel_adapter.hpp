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

#include "core/bitvector.hpp"
#include "ledger/state_adapter.hpp"
#include "vectorise/platform.hpp"

namespace fetch {
namespace ledger {

/**
 * Read / Write interface between the VM IO interface the and main ledger state database. Will
 * actively check to ensure reads and writes occur on permissible resources.
 */
class StateSentinelAdapter : public StateAdapter
{
public:
  using ResourceSet = TransactionSummary::ResourceSet;

  static constexpr char const *LOGGING_NAME = "StateSentinelAdapter";

  // Construction / Destruction
  StateSentinelAdapter(StorageInterface &storage, Identifier scope, BitVector const &shards);
  ~StateSentinelAdapter() override;

  /// @name IO Observer Interface
  /// @{
  Status Read(std::string const &key, void *data, uint64_t &size) override;
  Status Write(std::string const &key, void const *data, uint64_t size) override;
  Status Exists(std::string const &key) override;
  /// @}

  /// @name Counter Access
  /// @{
  uint64_t num_lookups() const
  {
    return lookups_;
  }
  uint64_t num_bytes_read() const
  {
    return bytes_read_;
  }
  uint64_t num_bytes_written() const
  {
    return bytes_written_;
  }
  /// @}

private:
  bool IsAllowedResource(std::string const &key) const;

  /// @name Shard Limits
  /// @{
  BitVector shards_;
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
