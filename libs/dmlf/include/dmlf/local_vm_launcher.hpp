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

  using Executable   = fetch::vm::Executable;
  using VM        = fetch::vm::VM;
  using VmFactory = fetch::vm_modules::VMFactory;
  using State     = VmPersistent;
  using Error = ExecutionResult::Error;

  ExecutionResult CreateExecutable(Name const &execName, SourceFiles const &sources) override;
  ExecutionResult DeleteExecutable(Name const &execName)                             override;

  ExecutionResult CreateState(Name const &stateName)                  override;
  ExecutionResult CopyState(Name const &srcName, Name const &newName) override;
  ExecutionResult DeleteState(Name const &stateName)                  override;

  ExecutionResult Run(Name const &execName, Name const &stateName,
                              std::string const &entrypoint) override;

private:
  bool HasExecutable(std::string const &name) const;
  bool HasState(std::string const &name) const;

  ExecutionResult EngineError(std::string resultMessage, Error::Code code, std::string errorMessage) const;
  ExecutionResult EngineSuccess(std::string resultMessage, std::string errorMessage) const;

  std::unordered_map<std::string, std::shared_ptr<Executable>> executables_;
  std::unordered_map<std::string, std::shared_ptr<State>>   states_;

  std::shared_ptr<fetch::vm::Module> module_ = VmFactory::GetModule(VmFactory::USE_SMART_CONTRACTS);
};

}  // namespace dmlf
}  // namespace fetch
