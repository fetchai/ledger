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

using Status                = SynergeticMinerScript::Status;
using SynergeticJob         = SynergeticMinerScript::SynergeticJob;
using SynergeticJobs        = SynergeticMinerScript::SynergeticJobs;
using VmStructuredData      = vm::Ptr<vm_modules::StructuredData>;
using VmStructuredDataArray = vm::Ptr<vm::Array<VmStructuredData>>;

constexpr char const *LOGGING_NAME = "SynergeticMinerScript";

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

VmStructuredData CreateSynergeticJobData(vm::VM *vm, SynergeticJob const &job)
{
  VmStructuredData data{};

  try
  {
    // create the structured data
    data =
        StructuredData::ConstructorFromVariant(vm, vm->GetTypeId<VmStructuredData>(), job);
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse input jobs data: ", ex.what());
  }

  return data;
}

VmStructuredDataArray CreateSynergeticJobData(vm::VM *vm, SynergeticJobs const &jobs)
{
  using UnderlyingArrayElement = vm::Array<VmStructuredData>::ElementType;
  using UnderlyingArray        = std::vector<UnderlyingArrayElement>;

  UnderlyingArray elements{};
  elements.reserve(jobs.size());

  for (auto const &job : jobs)
  {
    // convert the problem data
    auto data = CreateSynergeticJobData(vm, job);

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

SynergeticMinerScript::SynergeticMinerScript(ConstByteArray const &source)
  : module_{VMFactory::GetModule(VMFactory::USE_SMART_CONTRACTS)}
{
  // ensure the source has size
  if (source.empty())
  {
    throw std::runtime_error("Empty source for synergetic miner script");
  }

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
    case FunctionDecoratorKind::MINE_JOBS:
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
}

Status SynergeticMinerScript::GenerateJobList(SynergeticJobs const &jobs, JobList &generated_job_list)
{
  // create the VM
  auto vm  = std::make_unique<vm::VM>(module_.get());
  vm::Variant result;

  // create the problem data
  auto jobs_vm = CreateSynergeticJobData(vm.get(), jobs);

  // execute the problem definition function
  std::string error{};
  if (!vm->Execute(*executable_, mine_jobs_function_, error, result, jobs_vm))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Problem definition error: ", error);
    return Status::VM_EXECUTION_ERROR;
  }

  auto array{result.Get<vm::Ptr<vm::IArray>>()};

  generated_job_list.clear();
  for(int32_t i=0;i<array->Count();++i)
  {
    generated_job_list.push_back(array->PopFrontOne().Get<uint64_t>());
  }

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
