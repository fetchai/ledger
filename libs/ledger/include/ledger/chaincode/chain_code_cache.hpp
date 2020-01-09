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

#include "crypto/fnv.hpp"  // needed for std::hash<ConstByteArray>
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "meta/log2.hpp"

#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <utility>

namespace fetch {
namespace ledger {

class Contract;

class ChainCodeCache
{
public:
  using ContractPtr    = std::shared_ptr<Contract>;
  using StoragePtr     = ledger::StorageInterface;
  using ConstByteArray = byte_array::ConstByteArray;

  ContractPtr Lookup(ConstByteArray const &contract_id, StorageInterface &storage);

private:
  using Clock     = std::chrono::high_resolution_clock;
  using Timepoint = Clock::time_point;

  struct Element
  {
    explicit Element(ContractPtr c)
      : chain_code{std::move(c)}
    {}

    ContractPtr chain_code;
    Timepoint   timestamp{Clock::now()};
  };

  using UnderlyingCache = std::unordered_map<byte_array::ConstByteArray, Element>;

  static constexpr uint64_t CLEANUP_PERIOD = 16;
  static constexpr uint64_t CLEANUP_MASK   = CLEANUP_PERIOD - 1u;

  // Utils
  ContractPtr FindInCache(ConstByteArray const &contract_id);
  void        RunMaintenance();

  std::size_t     counter_{};
  UnderlyingCache cache_;

  static_assert(meta::IsLog2(CLEANUP_PERIOD), "Clean up period must be a valid power of 2");
};

}  // namespace ledger
}  // namespace fetch
