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

#include "dmlf/local_vm_launcher.hpp"

#include "dmlf/vm_persistent.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include <memory>
#include <sstream>

namespace fetch {
namespace dmlf {

namespace {
using Executable = LocalVmLauncher::Executable;
using VM      = LocalVmLauncher::VM;
using State   = LocalVmLauncher::State;

using VmFactory = fetch::vm_modules::VMFactory;
}  // namespace

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"

#pragma clang diagnostic ignored "-Wunused-parameter"
ExecutionResult LocalVmLauncher::CreateExecutable(Name const &execName, SourceFiles const &sources) 
{
  if (HasExecutable(execName))
  {
    return EngineError("Didn't create " + execName, Error::Code::BAD_EXECUTABLE, "Error: executable " + execName + " already exists.");
  }

  auto newExecutable = std::make_shared<Executable>();
  auto errors = fetch::vm_modules::VMFactory::Compile(module_, sources, *newExecutable);

  if (!errors.empty())
  {
    std::stringstream errorString;
    for (auto const& line : errors) 
    {
      errorString << line << '\n';
    }

    return ExecutionResult{Variant{}, Error{Error::Stage::COMPILE, Error::Code::COMPILATION_ERROR, errorString.str()}, "Compilation error: Didn't create " + execName};
  }

  programs_.emplace(execName, std::move(newExecutable));

  return ExecutionResult{Variant(), Error{Error::Stage::COMPILE, Error::Code::SUCCESS, ""}, "Created executable " + execName};
}
ExecutionResult LocalVmLauncher::DeleteExecutable(Name const &execName)                             
{
  return ExecutionResult{};
}

ExecutionResult LocalVmLauncher::CreateState(Name const &stateName)                  
{
  if (HasState(stateName))
  {
    return EngineError("Didn't create " + stateName, Error::Code::BAD_STATE, "Error: state " + stateName + " already exists.");
  }
  states_.emplace(stateName, std::make_shared<State>());
  return ExecutionResult{Variant(), Error{Error::Stage::ENGINE, Error::Code::SUCCESS, ""}, "Created state " + stateName};
}
ExecutionResult LocalVmLauncher::CopyState(Name const &srcName, Name const &newName) 
{
  if (!HasState(srcName))
  {
    return EngineError("Couldn't copy state " + srcName + " to " + newName, Error::Code::BAD_STATE, "Error: No state named " + srcName);
  }
  if (HasState(newName))
  {
    return EngineError("Couldn't copy state " + srcName + " to " + newName, Error::Code::BAD_DESTINATION, "Error: state " + srcName + " already exists.");
  }

  states_.emplace(std::move(newName), std::make_shared<State>(states_[srcName]->DeepCopy()));

  return EngineSuccess("Copied state " + srcName + " to " + newName, "");
}
ExecutionResult LocalVmLauncher::DeleteState(Name const &stateName)                  
{
  return ExecutionResult{};
}

ExecutionResult LocalVmLauncher::Run(Name const &execName, Name const &stateName,
    std::string const &entrypoint) 
{
  if (!HasExecutable(execName))
  {
    return EngineError("Could not run " + execName + " with state " + stateName, Error::Code::BAD_EXECUTABLE, "Error: No executable " + execName);
  }
  if (!HasState(stateName))
  {
    return EngineError("Could not run " + execName + " with state " + stateName, Error::Code::BAD_STATE, "Error: No state " + stateName);
  }

  auto &program = programs_[execName];
  auto &state   = states_[stateName];

  // We create a a VM for each execution. It might be better to create a single VM and reuse it, but (currently) if you create a VM before
  // compiling a program the VM is badly formed and crashes on execution
  VM vm(module_.get());
  vm.SetIOObserver(*state);
  //Remove this
  vm.AttachOutputDevice(VM::STDOUT, std::cout);

  //fetch::vm::ParameterPack parameterPack(vm->registered_types());
  //std::for_each(params.cbegin(), params.cend(),
  //              [&parameterPack](auto const &v) { parameterPack.AddSingle(v); });

  std::string        runTimeError;
  fetch::vm::Variant output;
  auto thereWasAnError = vm.Execute(*program, entrypoint, runTimeError, output);

  if (!runTimeError.empty())
  {
    return ExecutionResult{output, 
      Error{Error::Stage::RUNNING, Error::Code::RUNTIME_ERROR, std::move(runTimeError)}, 
      "Error running " + execName + " with state " + stateName}; 
  }

  return ExecutionResult{output, Error{Error::Stage::RUNNING, Error::Code::SUCCESS, ""}, "Ran " + execName + " with state " + stateName};
}

ExecutionResult LocalVmLauncher::EngineError(std::string resultMessage, Error::Code code, std::string errorMessage) const
{
  return ExecutionResult( Variant(), Error(Error::Stage::ENGINE, code, std::move(errorMessage)), std::move(resultMessage));
}

ExecutionResult LocalVmLauncher::EngineSuccess(std::string resultMessage, std::string errorMessage) const
{
  return ExecutionResult( Variant(), Error(Error::Stage::ENGINE, Error::Code::SUCCESS, std::move(errorMessage)), std::move(resultMessage));
}

#pragma clang diagnostic pop

bool LocalVmLauncher::HasExecutable(std::string const &name) const
{
  return programs_.find(name) != programs_.end();
}

bool LocalVmLauncher::HasState(std::string const &name) const
{
  return states_.find(name) != states_.end();
}

}  // namespace dmlf
}  // namespace fetch
