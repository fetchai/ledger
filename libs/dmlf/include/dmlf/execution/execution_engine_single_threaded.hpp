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

#include "dmlf/execution/execution_interface.hpp"
#include "dmlf/vm_persistent.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include <memory>
#include <sstream>
#include <unordered_map>

namespace fetch {
namespace dmlf {

class ExecutionEngineSingleThreaded : public ExecutionEngineInterface
{
public:
  ExecutionEngineSingleThreaded();
  virtual ~ExecutionEngineSingleThreaded();

  ExecutionEngineSingleThreaded(ExecutionEngineSingleThreaded const &other)  = delete;
  ExecutionEngineSingleThreaded(ExecutionEngineSingleThreaded const &&other) = delete;
  ExecutionEngineSingleThreaded &operator=(ExecutionEngineSingleThreaded const &other) = delete;
ExecutionEngineSingleThreaded &operator=(ExecutionEngineSingleThreaded &&other) = delete;

  using Name            = ExecutionEngineInterface::Name;
  using SourceFiles     = ExecutionEngineInterface::SourceFiles;
  using Target          = ExecutionEngineInterface::Target;
  using Variant         = ExecutionEngineInterface::Variant;
  using PromiseOfResult = ExecutionEngineInterface::PromiseOfResult;
  using Params          = ExecutionEngineInterface::Params;

  virtual PromiseOfResult CreateExecutable(Name const &       execName,
                                           SourceFiles const &sources) override;
  virtual PromiseOfResult DeleteExecutable(Target const &host, Name const &execName) override;

  virtual PromiseOfResult CreateState(Name const &stateName) override;
  virtual PromiseOfResult CopyState(Name const &srcName, Name const &newName) override;
  virtual PromiseOfResult DeleteState(Name const &stateName) override;

  virtual PromiseOfResult Run(Name const &execName, Name const &stateName,
                              std::string const &entrypoint) override;

private:
  using ErrorStage = ExecutionErrorMessage::Stage;
  using ErrorCode  = ExecutionErrorMessage::Code;
  using Error      = ExecutionResult::Error;

  using Executable       = fetch::vm::Executable;
  using VM               = fetch::vm::VM;
  using VmFactory        = fetch::vm_modules::VMFactory;
  using State            = VmPersistent;
  using PromiseFulfiller = std::shared_ptr<service::details::PromiseImplementation>;

  using ExecutablePtr = std::shared_ptr<Executable>;
  using ExecutableMap = std::unordered_map<std::string, ExecutablePtr>;
  using StatePtr      = std::shared_ptr<State>;
  using StateMap      = std::unordered_map<std::string, StatePtr>;

  ExecutableMap executables_;
  StateMap      states_;

  std::shared_ptr<fetch::vm::Module> module_ = VmFactory::GetModule(VmFactory::USE_SMART_CONTRACTS);
};

}  // namespace dmlf
}  // namespace fetch
