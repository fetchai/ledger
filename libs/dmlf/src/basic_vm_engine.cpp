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

#include "dmlf/execution/basic_vm_engine.hpp"

#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/common.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include <memory>
#include <sstream>

namespace fetch {
namespace dmlf {

// This must match the type as defined in variant::variant.hpp
using fp64_t = fetch::fixed_point::fp64_t;
using fp32_t = fetch::fixed_point::fp32_t;

ExecutionResult BasicVmEngine::CreateExecutable(Name const &execName, SourceFiles const &sources)
{
  if (HasExecutable(execName))
  {
    return EngineError(Error::Code::BAD_EXECUTABLE, "executable " + execName + " already exists.");
  }

  auto newExecutable = std::make_shared<Executable>();
  auto errors        = fetch::vm_modules::VMFactory::Compile(module_, sources, *newExecutable);

  if (!errors.empty())
  {
    std::stringstream errorString;
    for (auto const &line : errors)
    {
      errorString << line << '\n';
    }

    return ExecutionResult{
        LedgerVariant{},
        Error{Error::Stage::COMPILE, Error::Code::COMPILATION_ERROR, errorString.str()},
        std::string{}};
  }

  executables_.emplace(execName, std::move(newExecutable));
  return ExecutionResult{
      LedgerVariant(),
      Error{Error::Stage::COMPILE, Error::Code::SUCCESS, "Created executable " + execName},
      std::string{}};
}

ExecutionResult BasicVmEngine::DeleteExecutable(Name const &execName)
{
  auto it = executables_.find(execName);

  if (it == executables_.end())
  {
    return EngineError(Error::Code::BAD_EXECUTABLE, "executable " + execName + " does not exist.");
  }

  executables_.erase(it);
  return EngineSuccess("Deleted executable " + execName);
}

ExecutionResult BasicVmEngine::CreateState(Name const &stateName)
{
  if (HasState(stateName))
  {
    return EngineError(Error::Code::BAD_STATE, "state " + stateName + " already exists.");
  }

  states_.emplace(stateName, std::make_shared<State>());
  return ExecutionResult{
      LedgerVariant{},
      Error{Error::Stage::ENGINE, Error::Code::SUCCESS, "Created state " + stateName},
      std::string{}};
}

ExecutionResult BasicVmEngine::CopyState(Name const &srcName, Name const &newName)
{
  if (!HasState(srcName))
  {
    return EngineError(Error::Code::BAD_STATE, "No state named " + srcName);
  }
  if (HasState(newName))
  {
    return EngineError(Error::Code::BAD_DESTINATION, "state " + newName + " already exists.");
  }

  states_.emplace(newName, std::make_shared<State>(states_[srcName]->DeepCopy()));
  return EngineSuccess("Copied state " + srcName + " to " + newName);
}

ExecutionResult BasicVmEngine::DeleteState(Name const &stateName)
{
  auto it = states_.find(stateName);
  if (it == states_.end())
  {
    return EngineError(Error::Code::BAD_STATE, "No state named " + stateName);
  }

  states_.erase(it);
  return EngineSuccess("Deleted state " + stateName);
}

ExecutionResult BasicVmEngine::Run(Name const &execName, Name const &stateName,
                                   std::string const &entrypoint, Params params)
{
  if (!HasExecutable(execName))
  {
    return EngineError(Error::Code::BAD_EXECUTABLE, "No executable " + execName);
  }
  if (!HasState(stateName))
  {
    return EngineError(Error::Code::BAD_STATE, "No state " + stateName);
  }

  auto &             exec  = executables_[execName];
  auto &             state = states_[stateName];
  std::ostringstream console{};

  // We create a a VM for each execution. It might be better to create a single VM and reuse it, but
  // (currently) if you create a VM before compiling the VM is badly formed and crashes on execution
  VM vm{module_.get()};
  vm.SetIOObserver(*state);
  vm.AttachOutputDevice(fetch::vm::VM::STDOUT, console);

  // Convert and check function signature
  auto const *func = exec->FindFunction(entrypoint);

  if (func == nullptr)
  {
    return EngineError(Error::Code::RUNTIME_ERROR, entrypoint + " does not exist");
  }

  auto const numParameters = static_cast<std::size_t>(func->num_parameters);

  if (numParameters != params.size())
  {
    return EngineError(Error::Code::RUNTIME_ERROR,
                       "Wrong number of parameters expected " + std::to_string(numParameters) +
                           " recieved " + std::to_string(params.size()));
  }

  fetch::vm::ParameterPack parameterPack(vm.registered_types());
  for (std::size_t i = 0; i < numParameters; ++i)
  {
    auto const &typeId = func->variables[i].type_id;

    if (!Convertable(params[i], typeId))
    {
      std::cout << "GOT TYPE INFO\n";
      auto typeInfo = vm.GetTypeInfo(typeId);
      std::cout << typeInfo.name << '\n';
      std::cout << "END OF TYPE INFO\n";
      return EngineError(Error::Code::RUNTIME_ERROR, "Wrong parameter at " + std::to_string(i) +
                                                         " Expected " + vm.GetTypeName(typeId));
    }
    parameterPack.AddSingle(Convert(params[i], typeId));
  }

  // Run
  std::string runTimeError;
  VmVariant   vmOutput;

  bool allOK = vm.Execute(*exec, entrypoint, runTimeError, vmOutput, parameterPack);
  if (!allOK || !runTimeError.empty())
  {
    return ExecutionResult{
        LedgerVariant{},
        Error{Error::Stage::RUNNING, Error::Code::RUNTIME_ERROR, std::move(runTimeError)},
        console.str()};
  }

  return ExecutionResult{Convert(vmOutput),
                         Error{Error::Stage::RUNNING, Error::Code::SUCCESS,
                               "Ran " + execName + " with state " + stateName},
                         console.str()};
}

ExecutionResult BasicVmEngine::EngineError(Error::Code code, std::string errorMessage) const
{
  return ExecutionResult{LedgerVariant(),
                         Error{Error::Stage::ENGINE, code, std::move(errorMessage)}, std::string{}};
}

ExecutionResult BasicVmEngine::EngineSuccess(std::string successMessage) const
{
  return ExecutionResult{
      LedgerVariant(), Error{Error::Stage::ENGINE, Error::Code::SUCCESS, std::move(successMessage)},
      std::string{}};
}

bool BasicVmEngine::HasExecutable(std::string const &name) const
{
  return executables_.find(name) != executables_.end();
}

bool BasicVmEngine::HasState(std::string const &name) const
{
  return states_.find(name) != states_.end();
}

bool BasicVmEngine::Convertable(LedgerVariant const &ledgerVariant, TypeId const &typeId) const
{
  switch (typeId)
  {
  case fetch::vm::TypeIds::Bool:
  {
    return ledgerVariant.IsBoolean();
  }
  case fetch::vm::TypeIds::Int8:
  case fetch::vm::TypeIds::UInt8:
  case fetch::vm::TypeIds::Int16:
  case fetch::vm::TypeIds::UInt16:
  case fetch::vm::TypeIds::Int32:
  case fetch::vm::TypeIds::UInt32:
  case fetch::vm::TypeIds::Int64:
  {
    return ledgerVariant.IsInteger();
  }
  case fetch::vm::TypeIds::Float32:
  case fetch::vm::TypeIds::Float64:
  {
    return ledgerVariant.IsFloatingPoint();
  }
  case fetch::vm::TypeIds::Fixed32:
  case fetch::vm::TypeIds::Fixed64:
  {
    return ledgerVariant.IsFixedPoint();
  }
  default:
    std::cout << "Expected typeId " << typeId << '\n';
    return false;
  }
}
BasicVmEngine::VmVariant BasicVmEngine::Convert(LedgerVariant const &ledgerVariant,
                                                TypeId const &       typeId) const
{
  switch (typeId)
  {
  case fetch::vm::TypeIds::Bool:
  {
    return VmVariant(ledgerVariant.As<bool>(), typeId);
  }
  case fetch::vm::TypeIds::Int8:
  case fetch::vm::TypeIds::UInt8:
  case fetch::vm::TypeIds::Int16:
  case fetch::vm::TypeIds::UInt16:
  case fetch::vm::TypeIds::Int32:
  case fetch::vm::TypeIds::UInt32:
  case fetch::vm::TypeIds::Int64:
  {
    return VmVariant(ledgerVariant.As<int>(), typeId);
  }
  case fetch::vm::TypeIds::Float32:
  case fetch::vm::TypeIds::Float64:
  {
    return VmVariant(ledgerVariant.As<double>(), typeId);
  }
  case fetch::vm::TypeIds::Fixed32:
  case fetch::vm::TypeIds::Fixed64:
  {
    return VmVariant(ledgerVariant.As<fp64_t>(), typeId);
  }
  default:
    std::cout << "Tried to convert typeId " << typeId << '\n';
    return VmVariant();
  }
}

BasicVmEngine::LedgerVariant BasicVmEngine::Convert(VmVariant const &vmVariant) const
{
  switch (vmVariant.type_id)
  {
  case fetch::vm::TypeIds::Bool:
  {
    return LedgerVariant{vmVariant.Get<bool>()};
  }
  case fetch::vm::TypeIds::Int8:
  case fetch::vm::TypeIds::UInt8:
  case fetch::vm::TypeIds::Int16:
  case fetch::vm::TypeIds::UInt16:
  case fetch::vm::TypeIds::Int32:
  case fetch::vm::TypeIds::UInt32:
  case fetch::vm::TypeIds::Int64:
  {
    return LedgerVariant{vmVariant.Get<int>()};
  }
  case fetch::vm::TypeIds::Float32:
  case fetch::vm::TypeIds::Float64:
  {
    return LedgerVariant{vmVariant.Get<double>()};
  }
  case fetch::vm::TypeIds::Fixed32:
  {
    return LedgerVariant{vmVariant.Get<fp32_t>()};
  }
  case fetch::vm::TypeIds::Fixed64:
  {
    return LedgerVariant{vmVariant.Get<fp64_t>()};
  }

  default:
    return LedgerVariant{};
  }
}

}  // namespace dmlf
}  // namespace fetch
