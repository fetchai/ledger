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

#include "crypto/fnv.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chaincode/chain_code_cache.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/executor_interface.hpp"
#include "ledger/fees/fee_manager.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "ledger/transaction_validator.hpp"
#include "telemetry/telemetry.hpp"

#include <cstdint>
#include <memory>

namespace fetch {
namespace chain {

class Address;

}  // namespace chain
namespace ledger {

class CachedStorageAdapter;
class StateSentinelAdapter;
class StakeUpdateInterface;

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
  Result Execute(Digest const &digest, BlockIndex block, SliceIndex slice,
                 BitVector const &shards) override;
  void   SettleFees(chain::Address const &miner, BlockIndex block, TokenAmount amount,
                    uint32_t log2_num_lanes, StakeUpdateEvents const &stake_updates) override;
  /// @}

private:
  using TransactionPtr          = std::shared_ptr<chain::Transaction>;
  using CachedStorageAdapterPtr = std::shared_ptr<CachedStorageAdapter>;

  bool RetrieveTransaction(Digest const &digest);
  bool ValidationChecks(Result &result);
  bool ExecuteTransactionContract(Result &result);
  bool ProcessTransfers(Result &result);

  /// @name Resources
  /// @{
  StorageUnitPtr storage_;             ///< The collection of resources
  ChainCodeCache chain_code_cache_{};  //< The factory to create new chain code instances
  TokenContract  token_contract_{};
  /// @}

  /// @name Per Execution State
  /// @{
  BlockIndex              block_{};
  SliceIndex              slice_{};
  BitVector               allowed_shards_{};
  LaneIndex               log2_num_lanes_{0};
  TransactionPtr          current_tx_{};
  CachedStorageAdapterPtr storage_cache_;
  TransactionValidator    tx_validator_;
  /// @}

  FeeManager fee_manager_;

  telemetry::HistogramPtr overall_duration_;
  telemetry::HistogramPtr tx_retrieve_duration_;
  telemetry::HistogramPtr validation_checks_duration_;
  telemetry::HistogramPtr contract_execution_duration_;
  telemetry::HistogramPtr transfers_duration_;
  telemetry::HistogramPtr settle_fees_duration_;
};

}  // namespace ledger
}  // namespace fetch
