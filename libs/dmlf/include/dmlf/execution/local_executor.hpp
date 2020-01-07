#pragma once
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

#include "dmlf/execution/execution_engine_interface.hpp"
#include "dmlf/execution/execution_interface.hpp"

#include <memory>

namespace fetch {
namespace dmlf {

class LocalExecutor : public ExecutionInterface
{
public:
  using ExecutionEnginePtr = std::shared_ptr<ExecutionEngineInterface>;

  using Name            = ExecutionInterface::Name;
  using SourceFiles     = ExecutionInterface::SourceFiles;
  using Target          = ExecutionInterface::Target;
  using Variant         = ExecutionInterface::Variant;
  using PromiseOfResult = ExecutionInterface::PromiseOfResult;
  using Params          = ExecutionInterface::Params;

  using ErrorStage = ExecutionErrorMessage::Stage;
  using ErrorCode  = ExecutionErrorMessage::Code;
  using Error      = ExecutionResult::Error;

  explicit LocalExecutor(ExecutionEnginePtr exec_engine);
  ~LocalExecutor() override = default;

  LocalExecutor(LocalExecutor const &other) = delete;
  LocalExecutor(LocalExecutor &&other)      = delete;
  LocalExecutor &operator=(LocalExecutor const &other) = delete;
  LocalExecutor &operator=(LocalExecutor &&other) = delete;

  PromiseOfResult CreateExecutable(Target const &host, Name const &execName,
                                   SourceFiles const &sources) override;
  PromiseOfResult DeleteExecutable(Target const &host, Name const &execName) override;

  PromiseOfResult CreateState(Target const &host, Name const &stateName) override;
  PromiseOfResult CopyState(Target const &host, Name const &srcName, Name const &newName) override;
  PromiseOfResult DeleteState(Target const &host, Name const &stateName) override;

  PromiseOfResult Run(Target const &host, Name const &execName, Name const &stateName,
                      std::string const &entrypoint, Params const &params) override;

private:
  ExecutionEnginePtr exec_engine_;
};

}  // namespace dmlf
}  // namespace fetch
