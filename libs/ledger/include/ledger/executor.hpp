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

namespace v2 {
class Address;
}

class TokenContract;
class CachedStorageAdapter;
class StateSentinelAdapter;

/**
 * The executor object is designed to process incoming transactions
 */
class Executor : public ExecutorInterface
{
public:
  using StorageUnitPtr = std::shared_ptr<StorageUnitInterface>;
  using ConstByteArray = byte_array::ConstByteArray;

  // Construction / Destruction
  explicit Executor(StorageUnitPtr storage);
  ~Executor() override = default;

  /// @name Executor Interface
  /// @{
  Result Execute(v2::Digest const &digest, BlockIndex block, SliceIndex slice,
                 BitVector const &shards) override;
  void   SettleFees(v2::Address const &miner, TokenAmount amount, uint32_t log2_num_lanes) override;
  /// @}

private:
  using TokenContractPtr        = std::shared_ptr<TokenContract>;
  using TransactionPtr          = std::shared_ptr<v2::Transaction>;
  using CachedStorageAdapterPtr = std::shared_ptr<CachedStorageAdapter>;

  bool RetrieveTransaction(v2::Digest const &digest);
  bool ValidationChecks(Result &result);
  bool ExecuteTransactionContract(Result &result);
  bool ProcessTransfers(Result &result);
  void DeductFees(Result &result);
  bool Cleanup();

  /// @name Resources
  /// @{
  StorageUnitPtr   storage_;             ///< The collection of resources
  ChainCodeCache   chain_code_cache_{};  //< The factory to create new chain code instances
  TokenContractPtr token_contract_;
  /// @}

  /// @name Per Execution State
  /// @{
  BlockIndex              block_;
  SliceIndex              slice_;
  BitVector               allowed_shards_{};
  LaneIndex               log2_num_lanes_{0};
  TransactionPtr          current_tx_{};
  CachedStorageAdapterPtr storage_cache_;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
