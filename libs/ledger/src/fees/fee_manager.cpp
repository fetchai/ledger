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

#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/contract_context_attacher.hpp"
#include "ledger/execution_result.hpp"
#include "ledger/fees/fee_manager.hpp"
#include "ledger/state_sentinel_adapter.hpp"
#include "ledger/storage_unit/cached_storage_adapter.hpp"
#include "storage/resource_mapper.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/utils/timer.hpp"

using fetch::telemetry::Histogram;
using fetch::telemetry::Registry;
using fetch::storage::ResourceAddress;

namespace fetch {
namespace ledger {
namespace {

constexpr char const *LOGGING_NAME = "FeeManager";

}  // namespace

FeeManager::TransactionDetails::TransactionDetails(chain::Transaction &tx, BitVector const &shards)
  : from{tx.from()}
  , contract_address{tx.contract_address()}
  , shard_mask{shards}
  , digest{tx.digest()}
  , charge_rate{tx.charge_rate()}
  , charge_limit{tx.charge_limit()}
{}

FeeManager::TransactionDetails::TransactionDetails(chain::Address const &from_addr,
                                                   chain::Address const &contract_addr,
                                                   BitVector const &shards, Digest const &tx_digest,
                                                   TokenAmount const &rate,
                                                   TokenAmount const &limit)
  : from{from_addr}
  , contract_address{contract_addr}
  , shard_mask{shards}
  , digest{tx_digest}
  , charge_rate{rate}
  , charge_limit{limit}
{}

FeeManager::FeeManager(TokenContract &token_contract, std::string const &histogram_name)
  : token_contract_{token_contract}
  , deduct_fees_duration_{Registry::Instance().LookupMeasurement<Histogram>(histogram_name)}
{}

bool FeeManager::CalculateChargeAndValidate(TransactionDetails &             tx,
                                            std::vector<Chargeable *> const &chargeables,
                                            Result &                         result)
{
  bool success = true;

  uint64_t base_charge = 0;
  for (auto &chargeable : chargeables)
  {
    base_charge += chargeable->CalculateFee();
  }

  uint64_t const scaled_charge = std::max<uint64_t>(tx.shard_mask.PopCount(), 1) * base_charge;

  FETCH_LOG_DEBUG(LOGGING_NAME, "Calculated charge for 0x", tx.digest.ToHex(), ": ", scaled_charge,
                  " (base: ", base_charge, " storage: ", storage_charge,
                  " compute: ", compute_charge, " shards: ", allowed_shards_.PopCount(), ")");

  result.charge += scaled_charge;

  // determine if the chain code ran out of charge
  if (result.charge > tx.charge_limit)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Insufficient charge, charge (", result.charge,
                   ") greater then limit (", tx.charge_limit, ")");
    result.status = Status::INSUFFICIENT_CHARGE;
    success       = false;
  }

  if (success)
  {
    result.status = Status::SUCCESS;
  }

  return success;
}

void FeeManager::Execute(TransactionDetails &tx, Result &result, BlockIndex const &block,
                         StorageInterface &storage)
{
  telemetry::FunctionTimer const timer{*deduct_fees_duration_};

  // attach the token contract to the storage engine
  StateSentinelAdapter storage_adapter{storage, "fetch.token", tx.shard_mask};

  auto const &from = tx.from;

  ContractContext context{&token_contract_, tx.contract_address, nullptr, &storage_adapter, block};
  ContractContextAttacher raii(token_contract_, context);
  uint64_t const          balance = token_contract_.GetBalance(from);

  // calculate the fee to deduct
  TokenAmount tx_fee = result.charge * tx.charge_rate;
  if (Status::SUCCESS != result.status)
  {
    tx_fee = tx.charge_limit * tx.charge_rate;
  }

  // on failed transactions
  result.fee = std::min(balance, tx_fee);

  // deduct the fee from the originator
  token_contract_.SubtractTokens(from, result.fee);
}

void FeeManager::SettleFees(chain::Address const &miner, TokenAmount amount,
                            chain::Address const &contract_address, uint32_t log2_num_lanes,
                            BlockIndex const &block, StorageInterface &storage)
{
  // only if there are fees to settle then update the state database
  if (amount == 0)
  {
    return;
  }

  // compute the resource address
  ResourceAddress resource_address{"fetch.token.state." + miner.display()};

  // create the complete shard mask
  BitVector shard{1u << log2_num_lanes};
  shard.set(resource_address.lane(log2_num_lanes), 1);

  // attach the token contract to the storage engine
  StateSentinelAdapter storage_adapter{storage, "fetch.token", shard};

  ContractContext context{&token_contract_, contract_address, nullptr, &storage_adapter, block};
  ContractContextAttacher raii(token_contract_, context);
  token_contract_.AddTokens(miner, amount);
}

}  // namespace ledger
}  // namespace fetch
