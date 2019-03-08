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
#include "core/byte_array/encoders.hpp"
#include "crypto/fnv.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "ledger/chaincode/smart_contract_exception.hpp"
#include "ledger/chaincode/vm_definition.hpp"
#include "ledger/state_sentinel.hpp"
#include "ledger/storage_unit/cached_storage_adapter.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"
#include "vm_modules/vm_factory.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

using fetch::byte_array::ConstByteArray;

namespace fetch {
namespace ledger {
namespace {

enum class Kind
{
  NORMAL,  ///< Normal (undecorated) function
  ACTION,  ///< A Transaction handler
  QUERY,   ///< A Query handler
};

/**
 * Determine the type of the VM function
 *
 * @param fn The reference to function entry
 * @return The type of the function
 */
Kind DetermineKind(vm::Script::Function const &fn)
{
  Kind kind{Kind::NORMAL};

  // loop through all the function annotations
  if (1u == fn.annotations.size())
  {
    // select the first annotation
    auto const &annotation = fn.annotations.front();

    if (annotation.name == "@query")
    {
      // only update the kind if one hasn't already been specified
      kind = Kind::QUERY;
    }
    else if (annotation.name == "@action")
    {
      kind = Kind::ACTION;
    }
  }

  return kind;
}

/**
 * Compute the digest for the contract source
 *
 * @param source Reference to the contract source
 * @return The calculated digest of the source
 */
ConstByteArray GenerateDigest(std::string const &source)
{
  fetch::crypto::SHA256 hash;
  hash.Update(source);
  return hash.Final();
}

}  // namespace

/**
 * Construct a smart contract from the specified source
 *
 * @param source Reference to the script text
 */
SmartContract::SmartContract(std::string const &source)
  : source_{source}
  , digest_{GenerateDigest(source)}
  , script_{std::make_shared<Script>()}
  , module_{vm_modules::VMFactory::GetModule()}
{
  if (source_.empty())
  {
    throw SmartContractException(SmartContractException::Category::COMPILATION,
                                 {"No source present in contract"});
  }

  FETCH_LOG_WARN(LOGGING_NAME, "Constructing contract: ", contract_digest().ToBase64());

  // create and compile the script
  auto module = vm_modules::VMFactory::GetModule();
  auto errors = vm_modules::VMFactory::Compile(module, source_, *script_);

  // if there are any compilation errors
  if (!errors.empty())
  {
    throw SmartContractException(SmartContractException::Category::COMPILATION, std::move(errors));
  }

  // since we now have a fully compiled script we can evaluate the functions and assign the mapping

  // evaluate all the visible functions in this script and register the associated handle
  for (auto const &fn : script_->functions)
  {
    // determine the kind of function
    auto const kind = DetermineKind(fn);

    switch (kind)
    {
    case Kind::NORMAL:
      break;
    case Kind::ACTION:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Registering Action: ", fn.name,
                      " (Contract: ", contract_digest().ToBase64(), ')');

      // register the transaction handler
      OnTransaction(fn.name,
                    [this, name = fn.name](auto const &tx) { return InvokeAction(name, tx); });
      break;
    case Kind::QUERY:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Registering Query: ", fn.name,
                      " (Contract: ", contract_digest().ToBase64(), ')');

      // register the query handler
      OnQuery(fn.name, [this, name = fn.name](auto const &request, auto &response) {
        return InvokeQuery(name, request, response);
      });
      break;
    }
  }
}

/**
 * Invoke the specified action on the contract
 *
 * @param name The name of the action
 * @param tx The input transaction
 * @return The corresponding status result for the operation
 */
Contract::Status SmartContract::InvokeAction(std::string const &name, Transaction const &tx)
{
  variant::Variant data;
  if (!ParseAsJson(tx, data))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse data from SC. TX data: ", tx.data());
    /* malformatted TXs should cause block execution failure (although there is no guard for this)
     */
    return Status::FAILED;
  }

  FETCH_LOG_WARN(LOGGING_NAME, "Running smart contract target: ", name);

  // Get clean VM instance
  auto vm = vm_modules::VMFactory::GetVM(module_);
  vm->SetIOObserver(state());

  // Execute the requested function
  std::string        error;
  std::string        console;
  fetch::vm::Variant output;
  if (!vm->Execute(*script_, name, error, console, output))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Runtime error: ", error);
    return Status::FAILED;
  }

  return Status::OK;
}

/**
 * Invoke the specified query on the contract
 *
 * @param name The name of the query
 * @param request The query request
 * @param response The query response
 * @return The corresponding status result for the operation
 */
SmartContract::Status SmartContract::InvokeQuery(std::string const &name, Query const &request,
                                                 Query &response)
{
  FETCH_UNUSED(request);

  // get clean VM instance
  auto vm = vm_modules::VMFactory::GetVM(module_);
  vm->SetIOObserver(state());

  // create the initial query response
  response = Query::Object();

  vm::Variant output;
  std::string error;
  std::string console;
  if (!vm->Execute(*script_, name, error, console, output))
  {
    response["status"] = "failed";
    return Status::FAILED;
  }

  // extract the result from the contract output
  switch (output.type_id)
  {
  case vm::TypeIds::Bool:
    response["result"] = output.Get<bool>();
    break;
  case vm::TypeIds::Int8:
    response["result"] = output.Get<int8_t>();
    break;
  case vm::TypeIds::Byte:
    response["result"] = output.Get<uint8_t>();
    break;
  case vm::TypeIds::Int16:
    response["result"] = output.Get<int16_t>();
    break;
  case vm::TypeIds::UInt16:
    response["result"] = output.Get<uint16_t>();
    break;
  case vm::TypeIds::Int32:
    response["result"] = output.Get<int32_t>();
    break;
  case vm::TypeIds::UInt32:
    response["result"] = output.Get<uint32_t>();
    break;
  case vm::TypeIds::Int64:
    response["result"] = output.Get<int64_t>();
    break;
  case vm::TypeIds::UInt64:
    response["result"] = output.Get<uint64_t>();
    break;
  case vm::TypeIds::Float32:
    response["result"] = output.Get<float>();
    break;
  case vm::TypeIds::Float64:
    response["result"] = output.Get<double>();
    break;
  case vm::TypeIds::String:
    response["result"] = output.Get<vm::Ptr<vm::String>>()->str;
    break;
  default:
    break;
  }

  // update the status response to be successful
  response["status"] = "success";

  return Status::OK;
}

}  // namespace ledger
}  // namespace fetch
