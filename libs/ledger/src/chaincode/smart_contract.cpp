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
#include "ledger/state_adapter.hpp"
#include "ledger/storage_unit/cached_storage_adapter.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"
#include "vm/function_decorators.hpp"
#include "vm_modules/vm_factory.hpp"

#include "ledger/fetch_msgpack.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromBase64;

namespace fetch {
namespace ledger {

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
  auto errors = vm_modules::VMFactory::Compile(module_, source_, *script_);

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
    case vm::Kind::NORMAL:
      break;
    case vm::Kind::ON_INIT:
      break;
    case vm::Kind::ACTION:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Registering Action: ", fn.name,
                      " (Contract: ", contract_digest().ToBase64(), ')');

      // register the transaction handler
      OnTransaction(fn.name,
                    [this, name = fn.name](auto const &tx) { return InvokeAction(name, tx); });
      break;
    case vm::Kind::QUERY:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Registering Query: ", fn.name,
                      " (Contract: ", contract_digest().ToBase64(), ')');

      // register the query handler
      OnQuery(fn.name, [this, name = fn.name](auto const &request, auto &response) {
        return InvokeQuery(name, request, response);
      });
      break;

    case vm::Kind::INVALID:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Invalid function decorator found");
      throw SmartContractException(SmartContractException::Category::COMPILATION,
                                   {"Invalid decorator found in contract"});
      break;
    }
  }
}

/**
 * Extract the a given type from the container type and insert it into the parameter pack
 *
 * @tparam T The type to be extracted
 * @param pack The parameter pack to populate
 * @param obj The source object to extract the value from
 */
template <typename T>
void AddToParameterPack(vm::ParameterPack &pack, msgpack::object const &obj)
{
  // extract the value from the message pack object
  T value{};
  obj.convert(value);

  // add it to the parameter pack
  pack.Add(std::move(value));
}

/**
 * Extract the a given type from the container type and insert it into the parameter pack
 *
 * @tparam T The type to be extracted
 * @param pack The parameter pack to populate
 * @param obj The source object to extract the value from
 */
template <typename T>
void AddToParameterPack(vm::ParameterPack &pack, variant::Variant const &obj)
{
  // extract and add to the pack
  pack.Add(obj.As<T>());
}

/**
 * Convert a byte array into a std::vector
 *
 * @param buffer The reference to the buffer
 * @return The converted buffer
 */
std::vector<uint8_t> Convert(ConstByteArray const &buffer)
{
  return {buffer.pointer(), buffer.pointer() + buffer.size()};
}

/**
 * Extract an address from a msgpack::object
 *
 * @param vm The instance to the VM
 * @param pack The reference to the parameter pack to be populated
 * @param obj The object to extract the address from
 */
void AddAddressToParameterPack(vm::VM *vm, vm::ParameterPack &pack, msgpack::object const &obj)
{
  static int8_t const   ADDRESS_ID   = static_cast<int8_t>(0xAD);
  static uint32_t const ADDRESS_SIZE = 64u;

  bool valid{false};

  if (msgpack::type::EXT == obj.type)
  {
    auto const &ext = obj.via.ext;

    if ((ADDRESS_ID == ext.type()) && (ADDRESS_SIZE == ext.size))
    {
      uint8_t const *start = reinterpret_cast<uint8_t const *>(ext.data());
      uint8_t const *end   = start + ext.size;

      // create the instance of the address
      vm::Ptr<vm::Address> address = vm::Address::Constructor(vm, vm::TypeIds::Address);
      address->SetBytes(std::vector<uint8_t>{start, end});

      static_assert(vm::IsPtr<vm::Ptr<vm::Address>>::value, "");

      // add the address to the parameter pack
      pack.Add(std::move(address));
      valid = true;
    }
  }

  if (!valid)
  {
    throw std::runtime_error("Invalid address formart");
  }
}

/**
 * Extract an address from a JSON object
 *
 * @param vm The pointer to the VM
 * @param pack The parameter pack to be populated
 * @param obj The variant to extract the address from
 */
void AddAddressToParameterPack(vm::VM *vm, vm::ParameterPack &pack, variant::Variant const &obj)
{
  // create the instance of the address
  auto address = vm::Address::Constructor(vm, vm::TypeIds::Address);
  address->SetBytes(Convert(FromBase64(obj.As<ConstByteArray>())));

  // add it to the parameter list
  pack.Add(address);
}

/**
 * Extract a value from the variant type specified based on the expected type id and add it to the
 * given parameter pack
 *
 * @tparam T The variant type
 * @param params The parameter pack to be populated
 * @param expected_type The expected type to be extracted
 * @param variant The input variant from which the value is extracted
 */
template <typename T>
void AddToParameterPack(vm::VM *vm, vm::ParameterPack &params, vm::TypeId expected_type,
                        T const &variant)
{
  switch (expected_type)
  {
  case vm::TypeIds::Bool:
    AddToParameterPack<bool>(params, variant);
    break;

  case vm::TypeIds::Int8:
    AddToParameterPack<int8_t>(params, variant);
    break;

  case vm::TypeIds::Byte:
    AddToParameterPack<uint8_t>(params, variant);
    break;

  case vm::TypeIds::Int16:
    AddToParameterPack<int16_t>(params, variant);
    break;

  case vm::TypeIds::UInt16:
    AddToParameterPack<uint16_t>(params, variant);
    break;

  case vm::TypeIds::Int32:
    AddToParameterPack<int32_t>(params, variant);
    break;

  case vm::TypeIds::UInt32:
    AddToParameterPack<uint32_t>(params, variant);
    break;

  case vm::TypeIds::Int64:
    AddToParameterPack<int64_t>(params, variant);
    break;

  case vm::TypeIds::UInt64:
    AddToParameterPack<uint64_t>(params, variant);
    break;

  case vm::TypeIds::Address:
    AddAddressToParameterPack(vm, params, variant);
    break;

  default:
    throw std::runtime_error("Unable to map data type to VM entity");
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
  std::vector<msgpack::object> input_params;

  // if the tx has a payload parse it
  if (!tx.data().empty() && tx.data() != "{}")
  {
    // load the input data into msgpack for deserialisation
    msgpack::unpacker p;
    p.reserve_buffer(tx.data().size());
    std::memcpy(p.buffer(), tx.data().pointer(), tx.data().size());
    p.buffer_consumed(tx.data().size());

    msgpack::object_handle h;
    if (!p.next(h))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Parse error");
      return Status::FAILED;
    }

    auto const &container = h.get();

    if (msgpack::type::ARRAY != container.type)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Incorrect format, expected array of arguments. Input: ", tx.data());
      return Status::FAILED;
    }

    // access the elements of the array
    container.convert(input_params);
  }

  // Get clean VM instance
  auto vm = vm_modules::VMFactory::GetVM(module_);
  vm->SetIOObserver(state());

  // lookup the function / entry point which will be executed
  Script::Function const *target_function = script_->FindFunction(name);
  if (!target_function ||
      (input_params.size() != static_cast<std::size_t>(target_function->num_parameters)))
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Incorrect number of parameters provided for target function. Received: ",
                   input_params.size(), " Expected: ", target_function->num_parameters);
    return Status::FAILED;
  }

  vm::ParameterPack params{vm->registered_types()};

  // start to populate the parameter pack
  try
  {
    for (std::size_t i = 0; i < input_params.size(); ++i)
    {
      auto const &input_parameter    = input_params.at(i);
      auto const &expected_parameter = target_function->variables.at(i);

      AddToParameterPack(vm.get(), params, expected_parameter.type_id, input_parameter);
    }
  }
  catch (std::exception const &)
  {
    // this can happen for a number of reasons
    return Status::FAILED;
  }

  FETCH_LOG_WARN(LOGGING_NAME, "Running smart contract target: ", name);

  // Execute the requested function
  std::string        error;
  std::string        console;
  fetch::vm::Variant output;
  if (!vm->Execute(*script_, name, error, console, output, params))
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
  // get clean VM instance
  auto vm = vm_modules::VMFactory::GetVM(module_);
  vm->SetIOObserver(state());

  // lookup the script
  Script::Function const *target_function = script_->FindFunction(name);
  if (!target_function)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup target function");
    return Status::FAILED;
  }

  auto const num_parameters = static_cast<std::size_t>(target_function->num_parameters);

  // create and populate the parameter pack
  vm::ParameterPack params{vm->registered_types()};

  try
  {
    for (std::size_t i = 0; i < num_parameters; ++i)
    {
      auto const &parameter = target_function->variables[i];

      if (!request.Has(parameter.name))
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup variable: ", parameter.name);
        return Status::FAILED;
      }

      // add to the parameter pack
      AddToParameterPack(vm.get(), params, parameter.type_id, request[parameter.name]);
    }
  }
  catch (std::exception const &ex)
  {
    return Status::FAILED;
  }

  // create the initial query response
  response = Query::Object();

  vm::Variant output;
  std::string error;
  std::string console;
  if (!vm->Execute(*script_, name, error, console, output, params))
  {
    response["status"]  = "failed";
    response["msg"]     = error;
    response["console"] = console;
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
