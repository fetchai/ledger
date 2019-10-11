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

#include "dmlf/vm_launcher_interface.hpp"

#include "dmlf/vm_persistent.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include <memory>
#include <sstream>
#include <unordered_map>

namespace fetch {
namespace dmlf {

class LocalVmLauncher : public VmLauncherInterface
{
public:
  LocalVmLauncher()           = default;
  ~LocalVmLauncher() override = default;

  LocalVmLauncher(const LocalVmLauncher &other) = delete;
  LocalVmLauncher &operator=(const LocalVmLauncher &other)  = delete;
  bool             operator==(const LocalVmLauncher &other) = delete;
  bool             operator<(const LocalVmLauncher &other)  = delete;

  using Program   = fetch::vm::Executable;
  using VM        = fetch::vm::VM;
  using VmFactory = fetch::vm_modules::VMFactory;
  using State     = VmPersistent;

  bool CreateProgram(std::string name, std::string const &source) override;
  bool HasProgram(std::string const &name) const override;
  void AttachProgramErrorHandler(ProgramErrorHandler newHandler) override;

  bool CreateVM(std::string name) override;
  bool HasVM(std::string const &name) const override;
  bool SetVmStdout(std::string const &vmName, VmOutputHandler &newHandler) override;

  bool CreateState(std::string name) override;
  bool HasState(std::string const &name) const override;
  bool CopyState(std::string const &srcName, std::string newName) override;

  bool Execute(std::string const &programName, std::string const &vmName,
               std::string const &stateName, std::string const &entrypoint,
               Params const &params) override;
  void AttachExecuteErrorHandler(ExecuteErrorHandler newHandler) override;

  std::unordered_map<std::string, std::shared_ptr<VM>> vms_;

private:
  std::unordered_map<std::string, std::shared_ptr<Program>> programs_;
  std::unordered_map<std::string, std::shared_ptr<State>>   states_;

  ProgramErrorHandler programErrorHandler_ = nullptr;
  ExecuteErrorHandler executeErrorhandler_ = nullptr;

  std::shared_ptr<fetch::vm::Module> module_ = VmFactory::GetModule(VmFactory::USE_SMART_CONTRACTS);
};

}  // namespace dmlf
}  // namespace fetch
