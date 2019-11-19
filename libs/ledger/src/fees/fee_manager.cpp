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

#include "ledger/fees/fee_manager.hpp"
#include "ledger/execution_result.hpp"
#include "ledger/state_sentinel_adapter.hpp"
#include "ledger/storage_unit/cached_storage_adapter.hpp"
#include "ledger/execution_result.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/contract_context_attacher.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/utils/timer.hpp"

using fetch::telemetry::Histogram;
using fetch::telemetry::Registry;

namespace fetch {
namespace ledger {
namespace {
  constexpr char const *LOGGING_NAME = "FeeManager";

  bool IsCreateWealth(chain::Transaction const &tx)
  {
    return (tx.contract_mode() == chain::Transaction::ContractMode::CHAIN_CODE) &&
           (tx.chain_code() == "fetch.token") && (tx.action() == "wealth");
  }
}  // namespace

FeeManager::FeeManager(TokenContract &token_contract)
  : token_contract_{token_contract}
  , deduct_fees_duration_{Registry::Instance().LookupMeasurement<Histogram>(
      "ledger_fee_manager_deduct_fees_duration")}
{
}

bool FeeManager::CalculateChargeAndValidate(TransactionPtr& tx, std::vector<Chargeable*> const &chargeables, Result& result)
{
  bool success  = true;

  uint64_t base_charge = 0;
  for(auto &chargeable : chargeables)
  {
    base_charge += chargeable->CalculateFee();
  }

  uint64_t const scaled_charge =
      std::max<uint64_t>(tx->shard_mask().PopCount(), 1) * base_charge;

  FETCH_LOG_DEBUG(LOGGING_NAME, "Calculated charge for 0x", current_tx_->digest().ToHex(), ": ",
                  scaled_charge, " (base: ", base_charge, " storage: ", storage_charge,
                  " compute: ", compute_charge, " shards: ", allowed_shards_.PopCount(), ")");

  if (!IsCreateWealth(*tx))
  {
    result.charge += scaled_charge;
  }

  // determine if the chain code ran out of charge
  if (result.charge > tx->charge_limit())
  {
    result.status = Status::INSUFFICIENT_CHARGE;
    success       = false;
  }

  return success;
}


void FeeManager::Execute(TransactionPtr& tx, Result &result, BlockIndex &block, StorageInterface &storage)
{
  telemetry::FunctionTimer const timer{*deduct_fees_duration_};

  // attach the token contract to the storage engine
  StateSentinelAdapter storage_adapter{storage, Identifier{"fetch.token"}, tx->shard_mask()};

  auto const &from = tx->from();

  ContractContext context{&token_contract_, tx->contract_address(), &storage_adapter,
                          block};
  ContractContextAttacher raii(token_contract_, context);
  uint64_t const          balance = token_contract_.GetBalance(from);

  // calculate the fee to deduct
  TokenAmount tx_fee = result.charge * tx->charge();
  if (Status::SUCCESS != result.status)
  {
    tx_fee = tx->charge_limit() * tx->charge();
  }

  // on failed transactions
  result.fee = std::min(balance, tx_fee);

  // deduct the fee from the originator
  token_contract_.SubtractTokens(from, result.fee);
}

}  // namespace ledger
}  // namespace fetch