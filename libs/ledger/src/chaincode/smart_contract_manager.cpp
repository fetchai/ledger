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
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "crypto/fnv.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "ledger/chaincode/contract.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/contract_context_attacher.hpp"
#include "ledger/chaincode/smart_contract.hpp"
#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/chaincode/smart_contract_wrapper.hpp"
#include "logging/logging.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"
#include "vm/function_decorators.hpp"
#include "vm/module.hpp"
#include "vm_modules/vm_factory.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromBase64;
using fetch::vm::FunctionDecoratorKind;

namespace fetch {
namespace ledger {
namespace {

ConstByteArray const CONTRACT_SOURCE{"text"};
ConstByteArray const CONTRACT_HASH{"digest"};
ConstByteArray const CONTRACT_NONCE{"nonce"};

constexpr char const *LOGGING_NAME = "SmartContractManager";

}  // namespace

SmartContractManager::SmartContractManager()
{
  OnTransaction("create", this, &SmartContractManager::OnCreate);
}

Contract::Result SmartContractManager::OnCreate(chain::Transaction const &tx)
{
  // attempt to parse the transaction
  variant::Variant data;
  if (!ParseAsJson(tx, data))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "FAILED TO PARSE TRANSACTION");
    return {Status::FAILED};
  }

  ConstByteArray contract_source;
  ConstByteArray contract_hash;
  ConstByteArray nonce;

  // extract the fields from the contract
  bool const extract_success = Extract(data, CONTRACT_HASH, contract_hash) &&
                               Extract(data, CONTRACT_SOURCE, contract_source) &&
                               Extract(data, CONTRACT_NONCE, nonce);

  // fail if the extraction fails
  if (!extract_success)
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Failed to extract contract data from transaction body. Debug: ", contract_hash,
                   " : ", contract_source);
    return {Status::FAILED};
  }

  // decode the contents of the contract
  contract_source = FromBase64(contract_source);

  // debug
  FETCH_LOG_DEBUG(LOGGING_NAME, "---------------------------------------------------------------");
  FETCH_LOG_DEBUG(LOGGING_NAME, "New Contract Digest: ", contract_hash);
  FETCH_LOG_DEBUG(LOGGING_NAME, "Nonce..............: ", nonce);
  FETCH_LOG_DEBUG(LOGGING_NAME, "Text...............:\n\n", contract_source, "\n\n");
  FETCH_LOG_DEBUG(LOGGING_NAME, "---------------------------------------------------------------");

  // calculate a hash to compare against the one submitted
  auto const calculated_hash = crypto::Hash<crypto::SHA256>(contract_source).ToHex();

  if (calculated_hash != contract_hash)
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Warning! Failed to match calculated hash with provided hash: ", calculated_hash,
                   " to ", contract_hash);

    return {Status::FAILED};
  }

  if (tx.signatories().size() != 1)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Only one signature allowed when setting up smart contract");
    return {Status::FAILED};
  }

  nonce = FromBase64(nonce);
  chain::Address const contract_address{crypto::Hash<crypto::SHA256>(tx.from().address() + nonce)};

  SmartContractWrapper contract;
  if (GetStateRecord(contract, contract_address))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Contract ", contract_address.display(), " already created @ ",
                   contract.creation_timestamp);
    return {Status::OK};
  }

  // construct a smart contract - this can throw for various reasons, need to catch this
  SmartContract smart_contract{std::string{contract_source}};

  // Attempt to call the init method, if it exists
  std::string on_init_function;

  for (auto const &fn : smart_contract.executable()->functions)
  {
    // determine the kind of function
    auto const kind = DetermineKind(fn);

    switch (kind)
    {
    case FunctionDecoratorKind::ON_INIT:
      if (!on_init_function.empty())
      {
        FETCH_LOG_WARN(LOGGING_NAME, "More than one init function found in SC. Terminating.");
        return {Status::FAILED};
      }
      FETCH_LOG_DEBUG(LOGGING_NAME, "Found init function for SC");
      on_init_function = fn.name;
      break;

    case FunctionDecoratorKind::INVALID:
      FETCH_LOG_WARN(LOGGING_NAME, "Invalid function decorator found when adding SC");
      return {Status::FAILED};

    case FunctionDecoratorKind::ACTION:
    case FunctionDecoratorKind::NONE:
    case FunctionDecoratorKind::QUERY:
    case FunctionDecoratorKind::CLEAR:
    case FunctionDecoratorKind::OBJECTIVE:
    case FunctionDecoratorKind::PROBLEM:
    case FunctionDecoratorKind::WORK:
      break;
    }
  }

  // if there is an init function to run, do so.
  Result init_status;
  if (!on_init_function.empty())
  {
    state().PushContext(contract_address.display());

    {
      ContractContext ctx{context().token_contract, tx.contract_address(), nullptr, &state(),
                          context().block_index};
      ContractContextAttacher raii(smart_contract, ctx);
      init_status = smart_contract.DispatchInitialise(tx.from(), tx);
    }
    state().PopContext();

    if (init_status.status != Status::OK)
    {
      return init_status;
    }
  }
  auto const status = SetStateRecord(SmartContractWrapper{contract_source, init_status.block_index},
                                     contract_address);
  if (status != StateAdapter::Status::OK)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Failed to store smart contract to state DB!");
    init_status.status = Status::FAILED;
    return init_status;
  }

  init_status.status = Status::OK;
  return init_status;
}

/**
 * Used to generate resource address for the storage of the smart contract code
 *
 * @param contract_id The identifier for the smart contract being stored
 * @return The generated address
 */
storage::ResourceAddress SmartContractManager::CreateAddressForContract(
    chain::Address const &contract_id)
{
  // create the resource address in the form fetch.contract.state.<digest of contract>
  return StateAdapter::CreateAddress(NAME, contract_id.display());
}

}  // namespace ledger
}  // namespace fetch
