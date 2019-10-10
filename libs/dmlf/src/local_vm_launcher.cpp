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
  using VmOutputHandler = LocalVmLauncher::VmOutputHandler;
  using Params = LocalVmLauncher::Params;
  using ExecuteErrorHandler = LocalVmLauncher::ExecuteErrorHandler;

  using Program = LocalVmLauncher::Program;
  using VM = LocalVmLauncher::VM;
  using State = LocalVmLauncher::State;

  using VmFactory = fetch::vm_modules::VMFactory;
}

bool LocalVmLauncher::CreateProgram(std::string name, std::string const& source)
{
  if (HasProgram(name) ) 
  {
    return false;
  }

  fetch::vm::SourceFiles files = {{name+".etch", source}};
  auto newProgram = std::make_shared<Program>(); 
  auto errors = fetch::vm_modules::VMFactory::Compile(module_, files, *newProgram);

  if (!errors.empty() && programErrorHandler_ != nullptr) 
  {
    programErrorHandler_(name, errors);
    return false;
  }

  programs_.emplace(std::move(name), std::move(newProgram));

  return true; 
}
bool LocalVmLauncher::HasProgram(std::string const& name) const
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
bool LocalVmLauncher::HasVM(std::string const& name) const
{
  return vms_.find(name) != vms_.end();
}
bool LocalVmLauncher::SetVmStdout(std::string const& vmName, VmOutputHandler& newHandler)
{
  auto it = vms_.find(vmName);
  if (it == vms_.end()) {
    return false;
  }

  auto vm = it->second;
  vm->AttachOutputDevice(VM::STDOUT, newHandler);

  return true;
}

bool LocalVmLauncher::CreateState(std::string name)
{
  if (HasState(name)) {
    return false;
  }
  states_.emplace(std::move(name), std::make_shared<State>());
  return true;
}
bool LocalVmLauncher::HasState(std::string const& name) const
{
  return states_.find(name) != states_.end();
}
bool LocalVmLauncher::CopyState(std::string const& srcName, std::string newName)
{
  if (!HasState(srcName) || HasState(newName)) {
    return false;
  }

  states_.emplace(std::move(newName), std::make_shared<State>(states_[srcName]->DeepCopy()));

  return true;
}

bool LocalVmLauncher::Execute(std::string const& programName, std::string const& vmName, std::string const& stateName,
      std::string const& entrypoint, const Params /*params*/)
{
  if (!HasProgram(programName) || !HasVM(vmName) || !HasState(stateName)) {
    return false;
  }

  auto &program = programs_[programName];
  auto &vm = vms_[vmName];
  auto &state = states_[stateName];

  vm->SetIOObserver(*state);

  std::string runTimeError;
  fetch::vm::Variant output;
  auto thereWasAnError = vm->Execute(*program, entrypoint, runTimeError, output);

  if (!runTimeError.empty() && executeErrorhandler_ != nullptr) {
    executeErrorhandler_(programName, vmName, stateName, runTimeError);
    return false;
  }

  return thereWasAnError;
}
void LocalVmLauncher::AttachExecuteErrorHandler(ExecuteErrorHandler newHandler)
{
  executeErrorhandler_ = newHandler;
}

} // namespace dmlf
} // namespace fetch
