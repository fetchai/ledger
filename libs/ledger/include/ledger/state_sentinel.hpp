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
 * Read Only Adapter between the VM IO interface and the main ledger state database
 */
class StateAdapter : public vm::IoObserverInterface
{
public:
  using ConstByteArray  = byte_array::ConstByteArray;
  using ResourceAddress = storage::ResourceAddress;

  // Resource Mapping
  static ResourceAddress CreateAddress(Identifier const &scope, ConstByteArray const &key);

  // Construction / Destruction
  StateAdapter(StorageInterface &storage, Identifier scope);
  ~StateAdapter() override = default;

  /// @name Io Observer Interface
  /// @{
  Status Read(std::string const &key, void *data, uint64_t &size) override;
  Status Write(std::string const &key, void const *data, uint64_t size) override;
  Status Exists(std::string const &key) override;
  /// @}

protected:
  StorageInterface &storage_;
  Identifier        scope_;
};

/**
 * Read / Write interface between the VM IO interface the and main ledger state database. Will
 * actively check to ensure reads and writes occur on permissible resources.
 */
class StateSentinelAdapter : public StateAdapter
{
public:
  using ResourceSet = TransactionSummary::ResourceSet;

  // Construction / Destruction
  StateSentinelAdapter(StorageInterface &storage, Identifier scope, ResourceSet resources);
  ~StateSentinelAdapter() override;

  /// @name IO Observer Interface
  /// @{
  Status Read(std::string const &key, void *data, uint64_t &size) override;
  Status Write(std::string const &key, void const *data, uint64_t size) override;
  Status Exists(std::string const &key) override;
  /// @}

private:
  bool IsAllowedResource(std::string const &key) const;

  ResourceSet const resources_;
};

}  // namespace ledger
}  // namespace fetch