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

#include "ledger/chaincode/smart_contract.hpp"
#include "ledger/chaincode/database_interface.hpp"

#include "core/byte_array/decoders.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chaincode/vm_definition.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"

#include "core/byte_array/encoders.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"

#include "vm/vm_factory.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace fetch {
namespace ledger {

byte_array::ConstByteArray const CONTRACT_SOURCE{"contract_source_b64"};
byte_array::ConstByteArray const CONTRACT_HASH{"contract_hash_b64"};

SmartContract::SmartContract(byte_array::ConstByteArray const &identifier)
  : Contract(identifier)
{
  /* OnTransaction("main", this, &SmartContract::InvokeContract); */
}

//Contract::Status SmartContract::InvokeContract(Transaction const &tx, byte_array::ConstByteArray const &function_name)
Contract::Status SmartContract::InvokeContract(Transaction const &tx)
{
  variant::Variant data;
  if (!ParseAsJson(tx, data))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse data from SC. TX data: ", tx.data());
    return Status::FAILED;
  }

  byte_array::ConstByteArray contract_hash;    // Lookup
  byte_array::ByteArray      contract_source;  // Fill this

  if (!Extract(data, CONTRACT_HASH, contract_hash))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse contract hash from SC");
    return Status::FAILED;
  }

  if (!CheckRawState(contract_hash))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to find SC ", contract_hash," in main DB");
    return Status::FAILED;
  }

  if (source_.size() == 0)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Empty SC found! Shouldn't happen.");
  }

  FETCH_LOG_WARN(LOGGING_NAME, "Running smart contract");

  if (!RunSmartContract(source_, "main", contract_hash, tx))
  {
    // We're actually fine with this.
    return Status::OK;
  }

  return Status::OK;
}

bool SmartContract::RunSmartContract(std::string &source, std::string const &target_fn,
                                     byte_array::ConstByteArray const &hash, Transaction const &tx)
{
  FETCH_UNUSED(hash);
  char const *LOGGING_NAME = "RunSmartContract";
  auto        module       = vm::VMFactory::GetModule();

  // Compile source, get runnable script
  fetch::vm::Script        script;
  std::vector<std::string> errors = vm::VMFactory::Compile(module, source, script);

  for (auto const &error : errors)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Smart contract ", byte_array::ToHex(hash),
                   " encountered errors: ", error);
  }

  if (errors.size() > 0)
  {
    return false;
  }

  // Execute smart contract
  std::string        error;
  fetch::vm::Variant output;

  // Get clean VM instance
  auto vm = vm::VMFactory::GetVM(module);

  DatabaseInterface interface{this};

  for (auto const &resource : tx.resources())
  {
    interface.Allow(resource);
  }

  // Attach our state
  vm->SetIOInterface(&interface);

  // Execute our fn
  if (!vm->Execute(script, target_fn, error, print_strings_, output))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Runtime error: ", error);
  }

  // Successfully ran the VM. Write back the state
  if(allow_write_back_)
  {
    interface.WriteBackToState();
  }

  return true;
}

bool SmartContract::Exists(byte_array::ByteArray const &address)
{
  return StateRecordExists(address);
}

bool SmartContract::Get(byte_array::ByteArray &record, byte_array::ByteArray const &address)
{
  return GetStateRecord(record, address);
}

void SmartContract::Set(byte_array::ByteArray const &record, byte_array::ByteArray const &address)
{
  return SetStateRecord(record, address);
}

bool SmartContract::SetupHandlers()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Updating handlers for: ", identifier()[0]);

  byte_array::ByteArray      contract_source;

  if (source_.size() == 0)
  {
    if (!GetRawState(contract_source, identifier()[0]))
    {
      return false;
    }

    source_ = std::string{FromBase64(contract_source)};
  }

  auto        module       = vm::VMFactory::GetModule();
  fetch::vm::Script        script;
  std::vector<std::string> errors = vm::VMFactory::Compile(module, source_, script);

  if(errors.size() == 0)
  {
    for(auto const &fn : script.GetFunctions())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Attaching Fn: ", fn);
      OnTransaction(fn, this, &SmartContract::InvokeContract);
    }
  }
  else
  {
    return false;
  }

  return true;
}


}  // namespace ledger
}  // namespace fetch
