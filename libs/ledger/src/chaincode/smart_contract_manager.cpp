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

#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/chaincode/smart_contract.hpp"

#include "core/byte_array/decoders.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chaincode/vm_definition.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"

#include "vm/function_decorators.hpp"
#include "vm_modules/vm_factory.hpp"

#include "core/byte_array/encoders.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ToBase64;
using fetch::byte_array::FromBase64;

namespace fetch {
namespace ledger {

ConstByteArray const CONTRACT_SOURCE{"text"};
ConstByteArray const CONTRACT_HASH{"digest"};

SmartContractManager::SmartContractManager()
  : script_{std::make_shared<Script>()}
  , module_{vm_modules::VMFactory::GetModule()}
{
  OnTransaction("create", this, &SmartContractManager::OnCreate);
}

Contract::Status SmartContractManager::OnCreate(Transaction const &tx)
{
  // attempt to parse the transaction
  variant::Variant data;
  if (!ParseAsJson(tx, data))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "FAILED TO PARSE TRANSACTION");
    return Status::FAILED;
  }

  ConstByteArray contract_source;
  ConstByteArray contract_hash;

  // extract the fields from the contract
  bool const extract_success = Extract(data, CONTRACT_HASH, contract_hash) &&
                               Extract(data, CONTRACT_SOURCE, contract_source);

  // fail if the extraction fails
  if (!extract_success)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse contract source from transaction body");
    return Status::FAILED;
  }

  // decode the contents of the
  contract_source = FromBase64(contract_source);

  // calculate a hash to compare against the one submitted
  auto const calculated_hash = ToBase64(crypto::Hash<crypto::SHA256>(contract_source));

  if (calculated_hash != contract_hash)
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Warning! Failed to match calculated hash with provided hash: ", calculated_hash,
                   " to ", contract_hash);

    return Status::FAILED;
  }

  /*

  // Compile the script and run any initialization required
  auto errors = vm_modules::VMFactory::Compile(module_, std::string{contract_source}, *script_);

  // if there are any compilation errors
  if (!errors.empty())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to compile SC when adding it.");
    return Status::FAILED;
  }

  // since we now have a fully compiled script we can evaluate the functions and assign the mapping
  std::string on_init_function;

  // evaluate all the visible functions in this script and register the associated handle
  for (auto const &fn : script_->functions)
  {
    // determine the kind of function
    auto const kind = vm::DetermineKind(fn);

    switch (kind)
    {
    case vm::Kind::ON_INIT:
      if (on_init_function.size() > 0)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "More than one init function found in SC. Terminating.");
        return Status::FAILED;
      }
      FETCH_LOG_WARN(LOGGING_NAME, "Found init function for SC");
      on_init_function = fn.name;
      break;

    case vm::Kind::INVALID:
      FETCH_LOG_WARN(LOGGING_NAME, "Invalid function decorator found when adding SC");
      return Status::FAILED;

    default:
      break;
    }
  }

  auto tx_signatures = tx.signatures();

  if (tx_signatures.size() != 1)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Only one signature allowed when setting up smart contract");
    return Status::FAILED;
  }

  // Convert the TX signature to a base64
  auto pub_key_b64 = tx_signatures.begin()->first.identifier().ToBase64();

  Identifier scope;
  if (!scope.Parse(calculated_hash + "." + pub_key_b64))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse scope for smart contract");
    return Status::FAILED;
  }

  state().PushContext(scope);

  // Get clean VM instance
  auto vm = vm_modules::VMFactory::GetVM(module_);
  vm->SetIOObserver(state());

  FETCH_LOG_WARN(LOGGING_NAME, "Running SC init code: ", calculated_hash);

  // if there is an init function to run
  if (on_init_function.size() > 0)
  {
    vm::ParameterPack params{vm->registered_types()};

    // Execute the requested function
    std::string        error;
    std::string        console;
    fetch::vm::Variant output;
    if (!vm->Execute(*script_, on_init_function, error, console, output, params))
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Runtime error: ", error);
      return Status::FAILED;
    }
  }

  FETCH_LOG_WARN(LOGGING_NAME, "Adding smart contract, ID: ", calculated_hash,
                 " on_init called: ", on_init_function.size() != 0);

  state().PopContext();
  */

  // Set the scope for the smart contract to execute its on_init if it exists
  auto tx_signatures = tx.signatures();

  if (tx_signatures.size() != 1)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Only one signature allowed when setting up smart contract");
    return Status::FAILED;
  }

  Identifier scope;

  auto pub_key_b64 = tx_signatures.begin()->first.identifier().ToBase64();

  if (!scope.Parse(calculated_hash + "." + pub_key_b64))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse scope for smart contract");
    return Status::FAILED;
  }
  state().PushContext(scope);

  // construct a smart contract - this can throw for various reasons, need to catch this
  SmartContract smart_contract{std::string{contract_source}};

  // Attempt to call the init method, if it exists
  std::string on_init_function;

  for (auto const &fn : smart_contract.script()->functions)
  {
    // determine the kind of function
    auto const kind = DetermineKind(fn);

    switch (kind)
    {
    case vm::Kind::ON_INIT:
      if (on_init_function.size() > 0)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "More than one init function found in SC. Terminating.");
        return Status::FAILED;
      }
      FETCH_LOG_WARN(LOGGING_NAME, "Found init function for SC");
      on_init_function = fn.name;
      break;

    case vm::Kind::INVALID:
      FETCH_LOG_WARN(LOGGING_NAME, "Invalid function decorator found when adding SC");
      return Status::FAILED;

    default:
      break;
    }
  }

  // if there is an init function to run, do so.
  if (on_init_function.size() > 0)
  {
    // Attach our state to the smart contract
    smart_contract.Attach(state());

    // Dispatch to the init. method
    auto status = smart_contract.DispatchTransaction(on_init_function, tx);

    if (status != Status::OK)
    {
      return status;
    }

    smart_contract.Detach();
  }

  // Revert to normal context
  state().PopContext();

  auto success = true;
  SetStateRecord(contract_source, calculated_hash);

  if (!success)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Failed to store smart contract to state DB!");
    return Status::FAILED;
  }

  return Status::OK;
}

/**
 * Used to generate resource address for the storage of the smart contract code
 *
 * @param contract_id The identifier for the smart contract being stored
 * @return The generated address
 */
storage::ResourceAddress SmartContractManager::CreateAddressForContract(
    Identifier const &contract_id)
{
  // this function is only really applicable to the storage of smart contracts
  assert(contract_id.type() == Identifier::Type::SMART_CONTRACT);

  // create the resource address in the form fetch.contracts.state.<digest of contract>
  return StateAdapter::CreateAddress(Identifier{NAME}, contract_id.qualifier());
}

}  // namespace ledger
}  // namespace fetch
