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

byte_array::ConstByteArray const CONTRACT_SOURCE{"contract_source"};
byte_array::ConstByteArray const CONTRACT_HASH{"contract_hash"};

bool RunSmartContract(std::string &source, std::string const &target_fn,
                      byte_array::ConstByteArray const &hash);

SmartContract::SmartContract()
  : Contract("fetch.smart_contract")
{
  OnTransaction("main", this, &SmartContract::InvokeContract);
}

Contract::Status SmartContract::InvokeContract(Transaction const &tx)
{
  std::cerr << "in main here" << std::endl;

  variant::Variant data;
  if (!ParseAsJson(tx, data))
  {
    return Status::FAILED;
  }

  byte_array::ConstByteArray contract_hash;    // Lookup
  byte_array::ByteArray      contract_source;  // Fill this

  if (!Extract(data, CONTRACT_HASH, contract_hash))
  {
    return Status::FAILED;
  }

  if (!CheckRawState(contract_hash))
  {
    return Status::FAILED;
  }

  if (source_.size() == 0)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Source for SC not found. Populating.");

    if (!GetRawState(contract_source, contract_hash))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to lookup hash!", ToHex(contract_hash));
      return Status::FAILED;
    }

    source_ = std::string{contract_source};
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Using already existing SC cached.");
  }

  if (!RunSmartContract(source_, "main", contract_hash))
  {
    return Status::FAILED;
  }

  return Status::OK;
}

bool RunSmartContract(std::string &source, std::string const &target_fn,
                      byte_array::ConstByteArray const &hash)
{
  FETCH_UNUSED(hash);
  char const *LOGGING_NAME = "RunSmartContract";
  auto        module       = vm::VMFactory::GetModule();

  // ********** Attach way to interface with state here **********
  // module->state

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

  // Execute our fn
  if (!vm->Execute(script, target_fn, error, output))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Runtime error: ", error);
  }

  return true;
}

}  // namespace ledger
}  // namespace fetch
