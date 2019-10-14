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
using ProgramErrorHandler = LocalVmLauncher::ProgramErrorHandler;
using VmOutputHandler     = LocalVmLauncher::VmOutputHandler;
using Params              = LocalVmLauncher::Params;
using ExecuteErrorHandler = LocalVmLauncher::ExecuteErrorHandler;

using Program = LocalVmLauncher::Program;
using VM      = LocalVmLauncher::VM;
using State   = LocalVmLauncher::State;

using VmFactory = fetch::vm_modules::VMFactory;
}  // namespace

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"

#pragma clang diagnostic ignored "-Wunused-parameter"
ExecutionResult LocalVmLauncher::CreateExecutable(Name const &execName, SourceFiles const &sources) 
{
  if (HasProgram(execName))
  {
    return EngineError("Didn't create " + execName, Error::Code::BAD_EXECUTABLE, "Error: executable " + execName + " already exists.");
  }

  auto newProgram = std::make_shared<Program>();
  auto errors = fetch::vm_modules::VMFactory::Compile(module_, sources, *newProgram);

  if (!errors.empty())
  {
    std::stringstream errorString;
    for (auto const& line : errors) 
    {
      errorString << line << '\n';
    }

    return ExecutionResult{Variant{}, Error{Error::Stage::COMPILE, Error::Code::COMPILATION_ERROR, errorString.str()}, "Compilation error: Didn't create " + execName};
  }

  programs_.emplace(execName, std::move(newProgram));

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
  return ExecutionResult{};
}
ExecutionResult LocalVmLauncher::DeleteState(Name const &stateName)                  
{
  return ExecutionResult{};
}

ExecutionResult LocalVmLauncher::Run(Name const &execName, Name const &stateName,
    std::string const &entrypoint) 
{
  if (!HasProgram(execName))
  {
    return EngineError("Could not run " + execName + " with state " + stateName, Error::Code::BAD_EXECUTABLE, "Error: No executable " + execName);
  }
  if (!HasState(stateName))
  {
    return EngineError("Could not run " + execName + " with state " + stateName, Error::Code::BAD_STATE, "Error: No state " + stateName);
  }

  auto &program = programs_[execName];
  auto &state   = states_[stateName];

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

bool LocalVmLauncher::CreateProgram(std::string name, std::string const &source)
{
  if (HasProgram(name))
  {
    return false;
  }

  fetch::vm::SourceFiles files      = {{name + ".etch", source}};
  auto                   newProgram = std::make_shared<Program>();
  auto errors = fetch::vm_modules::VMFactory::Compile(module_, files, *newProgram);

  if (!errors.empty() && programErrorHandler_ != nullptr)
  {
    programErrorHandler_(name, errors);
    return false;
  }

  programs_.emplace(std::move(name), std::move(newProgram));

  return true;
}
bool LocalVmLauncher::HasProgram(std::string const &name) const
{
  return programs_.find(name) != programs_.end();
}
void LocalVmLauncher::AttachProgramErrorHandler(ProgramErrorHandler newHandler)
{
  programErrorHandler_ = newHandler;
}

bool LocalVmLauncher::CreateVM(std::string name)
{
  if (HasVM(name))
  {
    return false;
  }
  vms_.emplace(std::move(name), std::make_shared<VM>(module_.get()));
  return true;
}
bool LocalVmLauncher::HasVM(std::string const &name) const
{
  return vms_.find(name) != vms_.end();
}
bool LocalVmLauncher::SetVmStdout(std::string const &vmName, VmOutputHandler &newHandler)
{
  auto it = vms_.find(vmName);
  if (it == vms_.end())
  {
    return false;
  }

  auto vm = it->second;
  vm->AttachOutputDevice(VM::STDOUT, newHandler);

  return true;
}

bool LocalVmLauncher::CreateState2(std::string name)
{
  if (HasState(name))
  {
    return false;
  }
  states_.emplace(std::move(name), std::make_shared<State>());
  return true;
}
bool LocalVmLauncher::HasState(std::string const &name) const
{
  return states_.find(name) != states_.end();
}
bool LocalVmLauncher::CopyState2(std::string const &srcName, std::string newName)
{
  if (!HasState(srcName) || HasState(newName))
  {
    return false;
  }

  states_.emplace(std::move(newName), std::make_shared<State>(states_[srcName]->DeepCopy()));

  return true;
}

bool LocalVmLauncher::Execute(std::string const &programName, std::string const &vmName,
                              std::string const &stateName, std::string const &entrypoint,
                              Params const &params)
{
  if (!HasProgram(programName) || !HasVM(vmName) || !HasState(stateName))
  {
    return false;
  }

  auto &program = programs_[programName];
  auto &vm      = vms_[vmName];
  auto &state   = states_[stateName];

  vm->SetIOObserver(*state);

  fetch::vm::ParameterPack parameterPack(vm->registered_types());
  std::for_each(params.cbegin(), params.cend(),
                [&parameterPack](auto const &v) { parameterPack.AddSingle(v); });

  std::string        runTimeError;
  fetch::vm::Variant output;
  auto thereWasAnError = vm->Execute(*program, entrypoint, runTimeError, output, parameterPack);

  if (!runTimeError.empty() && executeErrorhandler_ != nullptr)
  {
    executeErrorhandler_(programName, vmName, stateName, runTimeError);
    return false;
  }

  return thereWasAnError;
}
void LocalVmLauncher::AttachExecuteErrorHandler(ExecuteErrorHandler newHandler)
{
  executeErrorhandler_ = newHandler;
}

}  // namespace dmlf
}  // namespace fetch
