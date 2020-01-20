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
#include "ledger/upow/synergetic_miner_script.hpp"
#include "logging/logging.hpp"
#include "vectorise/uint/uint.hpp"
#include "vm/address.hpp"
#include "vm/array.hpp"
#include "vm/compiler.hpp"
#include "vm/function_decorators.hpp"
#include "vm/vm.hpp"
#include "vm_modules/core/structured_data.hpp"
#include "vm_modules/ledger/synergetic_job.hpp"
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

using Status               = SynergeticMinerScript::Status;
using SynergeticJobs       = SynergeticMinerScript::SynergeticJobs;
using VmSynergeticJob      = vm::Ptr<vm_modules::ledger::SynergeticJob>;
using VmSynergeticJobArray = vm::Ptr<vm::Array<VmSynergeticJob>>;
using VmRandomUniform      = vm::Ptr<vm_modules::ledger::RandomUniform>;

constexpr char const *LOGGING_NAME = "SynergeticMinerScript";

constexpr uint64_t const HISTORY_SIZE = 10;

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

VmSynergeticJobArray CreateSynergeticJobData(vm::VM *vm, SynergeticJobs const &jobs)
{
  using UnderlyingArrayElement = vm::Array<VmSynergeticJob>::ElementType;
  using UnderlyingArray        = std::vector<UnderlyingArrayElement>;

  UnderlyingArray elements{};
  elements.reserve(jobs.size());

  for (auto const &job : jobs)
  {
    // convert the problem data
    auto data = job->ToVMType(vm);

    if (data)
    {
      elements.emplace_back(std::move(data));
    }
  }

  // create the array
  auto *ret = new vm::Array<VmSynergeticJob>(vm, vm->GetTypeId<vm::IArray>(),
                                             vm->GetTypeId<VmSynergeticJob>(),
                                             static_cast<int32_t>(elements.size()));

  // mov e the constructed elements over to the array
  ret->elements = std::move(elements);

  return VmSynergeticJobArray{ret};
}

}  // namespace

SynergeticMinerScript::SynergeticMinerScript(ConstByteArray const &source)
  : module_{VMFactory::GetModule(VMFactory::USE_SMART_CONTRACTS)}
  , history_{HISTORY_SIZE}
{
  // ensure the source has size
  if (source.empty())
  {
    throw std::runtime_error("Empty source for synergetic miner script");
  }

  vm_modules::ledger::SynergeticJob::Bind(*module_);
  vm_modules::ledger::SynergeticJobHistoryElement::Bind(*module_);
  vm_modules::ledger::RandomUniform::Bind(*module_);

  FETCH_LOG_DEBUG(LOGGING_NAME, "Synergetic miner script source\n", source);

  // create the compiler and IR
  compiler_   = std::make_shared<vm::Compiler>(module_.get());
  ir_         = std::make_shared<vm::IR>();
  executable_ = std::make_shared<vm::Executable>();

  // compile the source to IR
  std::vector<std::string> errors{};
  fetch::vm::SourceFiles   files = {{"default.etch", static_cast<std::string>(source)}};
  if (!compiler_->Compile(files, "default_ir", *ir_, errors))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to compile miner script: ", ErrorsToLog(errors));
    throw std::runtime_error("Failed to compile synergetic miner script");
  }

  // generate the executable
  auto vm = std::make_unique<vm::VM>(module_.get());
  if (!vm->GenerateExecutable(*ir_, "default_exe", *executable_, errors))
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Failed to generate executable for contract: ", ErrorsToLog(errors));
    throw std::runtime_error("Failed to generate executable for miner script");
  }

  // look through the contract and locate the required methods
  for (auto &f : executable_->functions)
  {
    auto const kind = DetermineKind(f);

    char const * name     = "unknown";
    std::string *function = nullptr;
    switch (kind)
    {
    case FunctionDecoratorKind::MINER_JOBS:
      name     = "mine_jobs";
      function = &mine_jobs_function_;
      break;
    case FunctionDecoratorKind::OBJECTIVE:
    case FunctionDecoratorKind::PROBLEM:
    case FunctionDecoratorKind::CLEAR:
    case FunctionDecoratorKind::NONE:
    case FunctionDecoratorKind::ON_INIT:
    case FunctionDecoratorKind::ACTION:
    case FunctionDecoratorKind::QUERY:
    case FunctionDecoratorKind::INVALID:
    case FunctionDecoratorKind ::WORK:
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

  vm_ = std::make_unique<vm::VM>(module_.get());

  random_unfiform_ = vm_->CreateNewObject<vm_modules::ledger::RandomUniform>();
}

void SynergeticMinerScript::set_balance(uint64_t const &balance)
{
  current_balance_ = balance;
}

void SynergeticMinerScript::set_back_expected_charge(int64_t const &charge)
{
  history_.back()->set_expected_charge(charge);
}

Status SynergeticMinerScript::GenerateJobList(SynergeticJobs const &jobs,
                                              JobList &generated_job_list, uint64_t balance)
{
  if (history_.size() > 0)
  {
    auto increment =
        static_cast<int64_t>(balance - current_balance_);  // < 0 then miner moved founds out
    // we are not handling found transfer in and out to the miner address
    history_.back()->set_actual_charge(increment);
  }
  current_balance_ = balance;

  // create the VM
  vm::Variant result;

  // create the problem data
  auto jobs_vm = CreateSynergeticJobData(vm_.get(), jobs);

  // execute the problem definition function
  std::string error{};
  if (!vm_->Execute(*executable_, mine_jobs_function_, error, result, jobs_vm,
                    history_.Get(vm_.get()), random_unfiform_))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Problem definition error: ", error);
    return Status::VM_EXECUTION_ERROR;
  }

  auto array{result.Get<vm::Ptr<vm::Array<uint64_t>>>()};

  FETCH_LOG_WARN(LOGGING_NAME, "Miner contract executed! Selected: ", array->Count());

  generated_job_list.clear();
  // int64_t expected_charge = 0;
  for (vm::AnyInteger i(0, vm::TypeIds::Int32); i.primitive.i32 < array->Count(); ++i.primitive.i32)
  {
    uint64_t id = array->GetIndexedValue(i).Get<uint64_t>();
    // expected_charge += static_cast<int64_t>(jobs[id]->total_charge());
    generated_job_list.push_back(id);
    FETCH_LOG_WARN(LOGGING_NAME, "Selected:  ", id);
  }

  history_.AddElement(vm_.get(), jobs_vm, array);

  return Status::SUCCESS;
}

char const *ToString(SynergeticMinerScript::Status status)
{
  char const *text = "Unknown";

  switch (status)
  {
  case SynergeticMinerScript::Status::SUCCESS:
    text = "Success";
    break;
  case SynergeticMinerScript::Status::VM_EXECUTION_ERROR:
    text = "VM Execution Error";
    break;
  case SynergeticMinerScript::Status::GENERAL_ERROR:
    text = "General Error";
    break;
  }

  return text;
}

}  // namespace ledger
}  // namespace fetch
