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

#include "chain/transaction.hpp"
#include "core/bitvector.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/fees/chargeable.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "telemetry/telemetry.hpp"

#include <vector>

namespace fetch {
namespace ledger {

class CachedStorageAdapter;
class StateSentinelAdapter;
enum class ContractExecutionStatus;
struct ContractExecutionResult;

class FeeManager
{
public:
  using CachedStorageAdapterPtr = std::shared_ptr<CachedStorageAdapter>;
  using TransactionPtr          = std::shared_ptr<chain::Transaction>;
  using BlockIndex              = uint64_t;
  using Status                  = ContractExecutionStatus;
  using Result                  = ContractExecutionResult;
  using TokenAmount             = uint64_t;

  struct TransactionDetails
  {
    TransactionDetails(chain::Transaction &tx, BitVector const &shards);
    TransactionDetails(chain::Address const &from_addr, chain::Address const &contract_addr,
                       BitVector const &shards, Digest const &tx_digest, TokenAmount const &rate,
                       TokenAmount const &limit);

    chain::Address const &from;
    chain::Address const &contract_address;
    BitVector const &     shard_mask;
    Digest const &        digest;
    TokenAmount const     charge_rate{1};
    TokenAmount const     charge_limit{0};
  };

  // Construction / Destruction
  FeeManager(TokenContract &token_contract, std::string const &histogram_name);
  FeeManager(FeeManager const &) = delete;
  FeeManager(FeeManager &&)      = delete;
  virtual ~FeeManager()          = default;

  bool CalculateChargeAndValidate(TransactionDetails &             tx,
                                  std::vector<Chargeable *> const &chargeables, Result &result);
  void Execute(TransactionDetails &tx, Result &result, BlockIndex const &block,
               StorageInterface &storage);
  void SettleFees(chain::Address const &miner, TokenAmount amount,
                  chain::Address const &contract_address, uint32_t log2_num_lanes,
                  BlockIndex const &block, StorageInterface &storage);

  // Operators
  FeeManager &operator=(FeeManager const &) = delete;
  FeeManager &operator=(FeeManager &&) = delete;

private:
  TokenContract &token_contract_;

  telemetry::HistogramPtr deduct_fees_duration_;
};

}  // namespace ledger
}  // namespace fetch
