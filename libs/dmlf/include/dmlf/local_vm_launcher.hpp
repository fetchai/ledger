#pragma once
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

#include "dmlf/execution/execution_engine_interface.hpp"

#include "dmlf/vm_persistent.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include <memory>
#include <sstream>
#include <unordered_map>

namespace fetch {
namespace dmlf {

class LocalVmLauncher : public ExecutionEngineInterface
{
public:
  LocalVmLauncher()           = default;
  ~LocalVmLauncher() override = default;

  LocalVmLauncher(const LocalVmLauncher &other) = delete;
  LocalVmLauncher &operator=(const LocalVmLauncher &other)  = delete;

  using Program   = fetch::vm::Executable;
  using VM        = fetch::vm::VM;
  using VmFactory = fetch::vm_modules::VMFactory;
  using State     = VmPersistent;

  // This will die
  using VmOutputHandler = std::ostream;
  using Params          = std::vector<fetch::vm::Variant>;
  // Program name, Error
  using ProgramErrorHandler = std::function<void(std::string const &, std::vector<std::string>)>;
  // Program name, VM name, State name, Error
  using ExecuteErrorHandler = std::function<void(std::string const &, std::string const &,
                                                 std::string const &, std::string const &)>;

  using Error = ExecutionResult::Error;

  ExecutionResult CreateExecutable(Name const &execName, SourceFiles const &sources) override;
  ExecutionResult DeleteExecutable(Name const &execName)                             override;

  ExecutionResult CreateState(Name const &stateName)                  override;
  ExecutionResult CopyState(Name const &srcName, Name const &newName) override;
  ExecutionResult DeleteState(Name const &stateName)                  override;

  ExecutionResult Run(Name const &execName, Name const &stateName,
                              std::string const &entrypoint) override;


  // This will also die
  bool CreateProgram(std::string name, std::string const &source) ;
  bool HasProgram(std::string const &name) const ;
  void AttachProgramErrorHandler(ProgramErrorHandler newHandler) ;

  bool CreateVM(std::string name) ;
  bool HasVM(std::string const &name) const ;
  bool SetVmStdout(std::string const &vmName, VmOutputHandler &newHandler) ;

  bool CreateState2(std::string name) ;
  bool HasState(std::string const &name) const ;
  bool CopyState2(std::string const &srcName, std::string newName) ;

  bool Execute(std::string const &programName, std::string const &vmName,
               std::string const &stateName, std::string const &entrypoint,
               Params const &params) ;
  void AttachExecuteErrorHandler(ExecuteErrorHandler newHandler) ;

private:

  ExecutionResult EngineError(std::string resultMessage, Error::Code code, std::string errorMessage) const;
  ExecutionResult EngineSuccess(std::string resultMessage, std::string errorMessage) const;

  std::unordered_map<std::string, std::shared_ptr<VM>> vms_;
  std::unordered_map<std::string, std::shared_ptr<Program>> programs_;
  std::unordered_map<std::string, std::shared_ptr<State>>   states_;

  ProgramErrorHandler programErrorHandler_ = nullptr;
  ExecuteErrorHandler executeErrorhandler_ = nullptr;

  std::shared_ptr<fetch::vm::Module> module_ = VmFactory::GetModule(VmFactory::USE_SMART_CONTRACTS);
};

}  // namespace dmlf
}  // namespace fetch
