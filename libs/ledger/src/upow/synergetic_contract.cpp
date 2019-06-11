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

#include "core/logger.hpp"
#include "ledger/upow/synergetic_contract.hpp"
#include "ledger/state_sentinel_adapter.hpp"
#include "ledger/storage_unit/cached_storage_adapter.hpp"
#include "vm/compiler.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"
#include "crypto/sha256.hpp"
#include "crypto/hash.hpp"

#include "vm_modules/math/bignumber.hpp"

#include <sstream>

namespace fetch {
namespace ledger {
namespace {

using vm_modules::BigNumberWrapper;
using vm_modules::VMFactory;
using byte_array::ConstByteArray;

using Status = SynergeticContract::Status;

constexpr char const *LOGGING_NAME = "SynContract";

enum class FunctionKind
{
  NORMAL,    ///< Normal (undecorated) function
  WORK,      ///< A function that is called to do some work
  OBJECTIVE, ///< A function that is called to determine the quality of the work
  PROBLEM,   ///< The problem function
  CLEAR,     ///< The clear function
  INVALID,   ///< The function has an invalid decorator
};

Digest ComputeDigest(ConstByteArray const &source)
{
  return crypto::Hash<crypto::SHA256>(source);
}

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

FunctionKind DetermineKind(vm::Executable::Function const &fn)
{
  FunctionKind kind{FunctionKind::NORMAL};

  // loop through all the function annotations
  if (1u == fn.annotations.size())
  {
    // select the first annotation
    auto const &annotation = fn.annotations.front();

    if (annotation.name == "@work")
    {
      // only update the kind if one hasn't already been specified
      kind = FunctionKind::WORK;
    }
    else if (annotation.name == "@objective")
    {
      kind = FunctionKind::OBJECTIVE;
    }
    else if (annotation.name == "@problem")
    {
      kind = FunctionKind::PROBLEM;
    }
    else if (annotation.name == "@clear")
    {
      kind = FunctionKind::CLEAR;
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Invalid decorator: ", annotation.name);
      kind = FunctionKind::INVALID;
    }
  }

  return kind;
}

}

SynergeticContract::SynergeticContract(ConstByteArray const &source)
  : digest_{ComputeDigest(source)}
  , module_{VMFactory::GetModule(VMFactory::USE_SYNERGETIC)}
{
  // ensure the source has size
  if (source.empty())
  {
    throw std::runtime_error("Empty source for synergetic contract");
  }

  FETCH_LOG_DEBUG(LOGGING_NAME, "Synergetic contract source\n", source);

  // additional modules

  // create the compiler and IR
  compiler_ = std::make_shared<vm::Compiler>(module_.get());
  ir_ = std::make_shared<vm::IR>();
  executable_ = std::make_shared<vm::Executable>();

  // compile the source to IR
  std::vector<std::string> errors{};
  if (!compiler_->Compile(static_cast<std::string>(source), "default", *ir_, errors))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to compile contract: ", ErrorsToLog(errors));
    throw std::runtime_error("Failed to compile synergetic contract");
  }

  // generate the executable
  auto vm = std::make_unique<vm::VM>(module_.get());
  if (!vm->GenerateExecutable(*ir_, "another", *executable_, errors))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to generate executable for contract: ", ErrorsToLog(errors));
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
    case FunctionKind::WORK:
      name     = "work";
      function = &work_function_;
      break;
    case FunctionKind::OBJECTIVE:
      name     = "objective";
      function = &objective_function_;
      break;
    case FunctionKind::PROBLEM:
      name     = "problem";
      function = &problem_function_;
      break;
    case FunctionKind::CLEAR:
      name     = "clear";
      function = &clear_function_;
      break;
    case FunctionKind::NORMAL:
    case FunctionKind::INVALID:
      break;
    }

    if (function)
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

Status SynergeticContract::DefineProblem()
{
  // create the VM
  auto vm = std::make_unique<vm::VM>(module_.get());
  problem_ = std::make_shared<vm::Variant>();

  // execute the problem definition function
  std::string error{};
  if (!vm->Execute(*executable_, problem_function_, error, *problem_))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Problem definition error: ", error);
    return Status::VM_EXECUTION_ERROR;
  }

  return Status::SUCCESS;
}

/**
 * Perform a piece of work based on a specified nonce
 *
 * @param nonce The nonce to be used to create the piece of work
 * @param score The score for the piece of work
 * @return The assoicated status for the operation
 */
Status SynergeticContract::Work(math::BigUnsigned const &nonce, WorkScore &score)
{
  // overriding assumption that the problem has previously been defined
  assert(static_cast<bool>(problem_));

  // assume this end badly
  score = std::numeric_limits<WorkScore>::max();

  auto vm = std::make_unique<vm::VM>(module_.get());

  // create the nonce object to be passed into the work function
  auto hashed_nonce = vm->CreateNewObject<BigNumberWrapper>(nonce);

  // execute the work function of the contract
  std::string error{};
  solution_ = std::make_shared<vm::Variant>();
  if (!vm->Execute(*executable_, work_function_, error, *solution_, *problem_, hashed_nonce))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Work execution error: ", error);
    return Status::VM_EXECUTION_ERROR;
  }

  // execute the objective function of the contract
  vm::Variant objective_output{};
  if (!vm->Execute(*executable_, objective_function_, error, objective_output, *problem_, *solution_))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Objective evaluation execution error: ", error);
    return Status::VM_EXECUTION_ERROR;
  }

  // ensure the output of the objective function is "correct"
  if (vm::TypeIds::Int64 != objective_output.type_id)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Incorrect objective value from function");
    return Status::VM_EXECUTION_ERROR;
  }

  // update the score
  score = objective_output.primitive.i64;

  return Status::SUCCESS;
}

Status SynergeticContract::Complete(uint64_t block, BitVector const &shards)
{
  if (!storage_)
  {
    return Status::NO_STATE_ACCESS;
  }

  auto vm = std::make_unique<vm::VM>(module_.get());

  // setup the storage infrastructure
  CachedStorageAdapter storage_cache(*storage_);
  StateSentinelAdapter state_sentinel{
      storage_cache, Identifier{digest_.ToHex() + "." + std::to_string(block)}, shards};

  // attach the state to the VM
  vm->SetIOObserver(state_sentinel);

  vm::Variant output;
  std::string error{};
  if (!vm->Execute(*executable_, clear_function_, error, output, *problem_, *solution_))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Solution execution failure: ", error);
    return Status::VM_EXECUTION_ERROR;
  }

  // everything worked, flush the storage
  storage_cache.Flush();

  return Status::SUCCESS;
}

} // namespace ledger
} // namespace fetch
