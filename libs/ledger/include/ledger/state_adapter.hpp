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

#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/identifier.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "vm/io_observer_interface.hpp"

#include <unordered_set>

namespace fetch {
namespace ledger {

/**
 * Adapter between the VM IO interface and the main ledger state database. This also keeps track
 * of the allowed accesses.
 */
class StateAdapter : public ::fetch::vm::IoObserverInterface
{
public:
  using ConstByteArray  = byte_array::ConstByteArray;
  using ResourceAddress = storage::ResourceAddress;
  using ResourceSet = std::set<byte_array::ConstByteArray>;

  static constexpr char const *LOGGING_NAME = "StateAdapter";

  // Resource Mapping
  static ResourceAddress CreateAddress(Identifier const &scope, ConstByteArray const &key);
  static ResourceAddress CreateAddress(ConstByteArray const &key);

  // Construction / Destruction
  StateAdapter(StorageInterface &storage, Identifier scope, ResourceSet resources, ResourceSet contract_hashes);
  StateAdapter(StorageInterface &storage, Identifier scope, ResourceSet resources);
  StateAdapter(StorageInterface &storage, Identifier scope);
  ~StateAdapter() override = default;

  /// @name Io Observer Interface
  /// @{
  Status Read(std::string const &key, void *data, uint64_t &size) override;
  Status Write(std::string const &key, void const *data, uint64_t size) override;
  Status Exists(std::string const &key) override;
  /// @}

  bool             query_mode = false;
  void PushContext(Identifier const &scope);
  void PopContext();

protected:
  StorageInterface &storage_;
  std::vector<Identifier> scope_;

  bool IsAllowedResource(std::string const &key) const;
  std::set<std::string> allowed_accesses_;
};

}  // namespace ledger
}  // namespace fetch
