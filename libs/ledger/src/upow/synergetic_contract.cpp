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

#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "json/document.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/fees/storage_fee.hpp"
#include "ledger/state_sentinel_adapter.hpp"
#include "ledger/storage_unit/cached_storage_adapter.hpp"
#include "ledger/upow/synergetic_contract.hpp"
#include "logging/logging.hpp"
#include "vectorise/uint/uint.hpp"
#include "vm/address.hpp"
#include "vm/array.hpp"
#include "vm/compiler.hpp"
#include "vm/function_decorators.hpp"
#include "vm/vm.hpp"
#include "vm_modules/core/structured_data.hpp"
#include "vm_modules/ledger/balance.hpp"
#include "vm_modules/ledger/transfer_function.hpp"
#include "vm_modules/math/bignumber.hpp"
#include "vm_modules/vm_factory.hpp"

#include <sstream>
#include <stdexcept>
#include <string>

namespace fetch {
namespace ledger {
namespace {

using vm::FunctionDecoratorKind;
using vm_modules::math::UInt256Wrapper;
using vm_modules::VMFactory;
using vm_modules::StructuredData;
using byte_array::ConstByteArray;
using crypto::Hash;
using crypto::SHA256;

using Status                = SynergeticContract::Status;
using ProblemData           = SynergeticContract::ProblemData;
using VmStructuredData      = vm::Ptr<vm_modules::StructuredData>;
using VmStructuredDataArray = vm::Ptr<vm::Array<VmStructuredData>>;

constexpr char const *LOGGING_NAME = "SynergeticContract";

std::string ErrorsToLog(std::vector<std::string> const &errors)
{
  // generate the complete error
  std::ostringstream oss;
  for (auto const &err : errors)
  {
    oss << '\n' << err;
  }

  return oss.str();
}

VmStructuredData CreateProblemData(vm::VM *vm, ConstByteArray const &problem_data)
{
  VmStructuredData data{};

  try
  {
    // parse the input data
    json::JSONDocument doc{problem_data};

    // create the structured data
    data =
        StructuredData::ConstructorFromVariant(vm, vm->GetTypeId<VmStructuredData>(), doc.root());
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse input problem data: ", ex.what());
  }

  return data;
}

VmStructuredDataArray CreateProblemData(vm::VM *vm, ProblemData const &problem_data)
{
  using UnderlyingArrayElement = vm::Array<VmStructuredData>::ElementType;
  using UnderlyingArray        = std::vector<UnderlyingArrayElement>;

  UnderlyingArray elements{};
  elements.reserve(problem_data.size());

  for (auto const &problem : problem_data)
  {
    // convert the problem data
    auto data = CreateProblemData(vm, problem);

    if (data)
    {
      elements.emplace_back(std::move(data));
    }
  }

  // create the array
  auto *ret = new vm::Array<VmStructuredData>(vm, vm->GetTypeId<vm::IArray>(),
                                              vm->GetTypeId<VmStructuredData>(),
                                              static_cast<int32_t>(elements.size()));

  // move the constructed elements over to the array
  ret->elements = std::move(elements);

  return VmStructuredDataArray{ret};
}

}  // namespace

SynergeticContract::SynergeticContract(ConstByteArray const &source)
  : digest_{Hash<SHA256>(source)}
  , module_{VMFactory::GetModule(VMFactory::USE_SMART_CONTRACTS)}
{
  // ensure the source has size
  if (source.empty())
  {
    throw std::runtime_error("Empty source for synergetic contract");
  }

  FETCH_LOG_DEBUG(LOGGING_NAME, "Synergetic contract source\n", source);

  vm_modules::ledger::BindBalanceFunction(*module_, *this);
  vm_modules::ledger::BindTransferFunction(*module_, *this);

  // create the compiler and IR
  compiler_   = std::make_shared<vm::Compiler>(module_.get());
  ir_         = std::make_shared<vm::IR>();
  executable_ = std::make_shared<vm::Executable>();

  // compile the source to IR
  std::vector<std::string> errors{};
  fetch::vm::SourceFiles   files = {{"default.etch", static_cast<std::string>(source)}};
  if (!compiler_->Compile(files, "default_ir", *ir_, errors))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to compile contract: ", ErrorsToLog(errors));
    throw std::runtime_error("Failed to compile synergetic contract");
  }

  // generate the executable
  auto vm = std::make_unique<vm::VM>(module_.get());
  if (!vm->GenerateExecutable(*ir_, "default_exe", *executable_, errors))
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Failed to generate executable for contract: ", ErrorsToLog(errors));
    throw std::runtime_error("Failed to generate executable for contract");
  }

  // look through the contract and locate the required methods
  for (auto &f : executable_->functions)
  {
    auto const kind = DetermineKind(f);

    char const * name     = "unknown";
    std::string *function = nullptr;
    switch (kind)
    {
    case FunctionDecoratorKind::WORK:
      name     = "work";
      function = &work_function_;
      break;
    case FunctionDecoratorKind::OBJECTIVE:
      name     = "objective";
      function = &objective_function_;
      break;
    case FunctionDecoratorKind::PROBLEM:
      name     = "problem";
      function = &problem_function_;
      break;
    case FunctionDecoratorKind::CLEAR:
      name     = "clear";
      function = &clear_function_;
      break;
    case FunctionDecoratorKind::NONE:
    case FunctionDecoratorKind::ON_INIT:
    case FunctionDecoratorKind::ACTION:
    case FunctionDecoratorKind::QUERY:
    case FunctionDecoratorKind::INVALID:
      break;
    }

    if (function != nullptr)
    {
      // sanity check
      assert(name != nullptr);

      if (!function->empty())
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Duplicate ", name, " handlers not permitted");
        throw std::runtime_error("Duplicate handlers");
      }

      // update the function name
      *function = f.name;
    }
  }
}

Status SynergeticContract::DefineProblem(ProblemData const &problem_data)
{
  // create the VM
  auto vm  = std::make_unique<vm::VM>(module_.get());
  problem_ = std::make_shared<vm::Variant>();

  if (charge_limit_ > 0)
  {
    vm->SetChargeLimit(charge_limit_);
  }

  // create the problem data
  auto problems = CreateProblemData(vm.get(), problem_data);

  // execute the problem definition function
  std::string error{};
  if (!vm->Execute(*executable_, problem_function_, error, *problem_, problems))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Problem definition error: ", error);
    return Status::VM_EXECUTION_ERROR;
  }

  charge_ += vm->GetChargeTotal();

  return Status::SUCCESS;
}

/**
 * Perform a piece of work based on a specified nonce
 *
 * @param nonce The nonce to be used to create the piece of work
 * @param score The score for the piece of work
 * @return The assoicated status for the operation
 */
Status SynergeticContract::Work(vectorise::UInt<256> const &nonce, WorkScore &score)
{
  // overriding assumption that the problem has previously been defined
  assert(static_cast<bool>(problem_));

  // assume this end badly
  score = std::numeric_limits<WorkScore>::max();

  auto vm = std::make_unique<vm::VM>(module_.get());

  if (charge_limit_ > 0)
  {
    vm->SetChargeLimit(charge_limit_);
  }

  // create the nonce object to be passed into the work function
  auto hashed_nonce = vm->CreateNewObject<UInt256Wrapper>(nonce);

  // execute the work function of the contract
  std::string error{};
  solution_ = std::make_shared<vm::Variant>();
  if (!vm->Execute(*executable_, work_function_, error, *solution_, *problem_, hashed_nonce))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Work execution error: ", error);
    charge_ += vm->GetChargeTotal();
    return Status::VM_EXECUTION_ERROR;
  }

  // execute the objective function of the contract
  vm::Variant objective_output{};
  if (!vm->Execute(*executable_, objective_function_, error, objective_output, *problem_,
                   *solution_))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Objective evaluation execution error: ", error);
    charge_ += vm->GetChargeTotal();
    return Status::VM_EXECUTION_ERROR;
  }

  charge_ += vm->GetChargeTotal();

  // ensure the output of the objective function is "correct"
  if (vm::TypeIds::Int64 != objective_output.type_id)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Objective function must return Int64");
    return Status::VM_EXECUTION_ERROR;
  }

  // update the score
  score = objective_output.primitive.i64;

  return Status::SUCCESS;
}

Status SynergeticContract::Complete(chain::Address const &address, BitVector const &shards,
                                    CompletionValidator const &validator)
{
  if (storage_ == nullptr)
  {
    return Status::NO_STATE_ACCESS;
  }

  auto vm = std::make_unique<vm::VM>(module_.get());

  if (charge_limit_ > 0)
  {
    vm->SetChargeLimit(charge_limit_);
  }

  // setup the storage infrastructure
  CachedStorageAdapter storage_cache(*storage_);
  StateSentinelAdapter state_sentinel{storage_cache, address.display(), shards};

  // attach the state to the VM
  vm->SetIOObserver(state_sentinel);

  vm::Variant output;
  std::string error{};
  if (!vm->Execute(*executable_, clear_function_, error, output, *problem_, *solution_))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Solution execution failure: ", error);
    charge_ += vm->GetChargeTotal();
    return Status::VM_EXECUTION_ERROR;
  }

  charge_ += vm->GetChargeTotal();

  StorageFee storage_fee{state_sentinel};
  charge_ += storage_fee.CalculateFee();

  if (!validator())
  {
    storage_cache.Clear();
    return Status::VALIDATION_ERROR;
  }

  // everything worked, flush the storage
  storage_cache.Flush();

  return Status::SUCCESS;
}

uint64_t SynergeticContract::CalculateFee() const
{
  return charge_;
}

void SynergeticContract::SetChargeLimit(uint64_t charge_limit)
{
  charge_limit_ = charge_limit;
}

bool SynergeticContract::HasProblem() const
{
  return static_cast<bool>(problem_);
}

vm::Variant const &SynergeticContract::GetProblem() const
{
  if (!problem_)
  {
    throw std::runtime_error("The contract does not have a problem");
  }

  return *problem_;
}

bool SynergeticContract::HasSolution() const
{
  return static_cast<bool>(solution_);
}

vm::Variant const &SynergeticContract::GetSolution() const
{
  if (!solution_)
  {
    throw std::runtime_error("The contract");
  }

  return *solution_;
}

Digest const &SynergeticContract::digest() const
{
  return digest_;
}

std::string const &SynergeticContract::work_function() const
{
  return work_function_;
}

std::string const &SynergeticContract::problem_function() const
{
  return problem_function_;
}

std::string const &SynergeticContract::objective_function() const
{
  return objective_function_;
}

std::string const &SynergeticContract::clear_function() const
{
  return clear_function_;
}

void SynergeticContract::Attach(StorageInterface &storage)
{
  storage_ = &storage;
}

void SynergeticContract::Detach()
{
  storage_ = nullptr;
  problem_.reset();
  solution_.reset();
  charge_       = 0;
  charge_limit_ = 0;
}

char const *ToString(SynergeticContract::Status status)
{
  char const *text = "Unknown";

  switch (status)
  {
  case SynergeticContract::Status::SUCCESS:
    text = "Success";
    break;
  case SynergeticContract::Status::VM_EXECUTION_ERROR:
    text = "VM Execution Error";
    break;
  case SynergeticContract::Status::NO_STATE_ACCESS:
    text = "No State Access";
    break;
  case SynergeticContract::Status::GENERAL_ERROR:
    text = "General Error";
    break;
  case SynergeticContract::Status::VALIDATION_ERROR:
    text = "Failed to validate";
    break;
  }

  return text;
}

void SynergeticContract::UpdateContractContext(ContractContext const &context)
{
  context_ = std::make_unique<ContractContext>(context);
}

ContractContext const &SynergeticContract::context() const
{
  return *context_;
}

}  // namespace ledger
}  // namespace fetch
