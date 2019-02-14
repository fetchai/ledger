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

#include "crypto/fnv.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chaincode/chain_code_cache.hpp"
#include "ledger/executor_interface.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <unordered_set>
#include <vector>

namespace fetch {
namespace ledger {

/**
 * The executor object is designed to process incoming transactions
 */
class Executor : public ExecutorInterface
{
public:
  using Resources = std::shared_ptr<StorageUnitInterface>;

  // Construction / Destruction
  explicit Executor(Resources resources)
    : resources_{std::move(resources)}
  {}
  ~Executor() override = default;

  /// @name Executor Interface
  /// @{
  Status Execute(TxDigest const &hash, std::size_t slice, LaneSet const &lanes) override;
  /// @}

private:
  Resources      resources_;         ///< The collection of resources
  ChainCodeCache chain_code_cache_;  //< The factory to create new chain code instances
};

}  // namespace ledger
}  // namespace fetch
