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

#include "dmlf/execution/execution_params.hpp"
#include "dmlf/execution/execution_result.hpp"
#include "vm/common.hpp"

#include <string>
#include <vector>

namespace fetch {
namespace dmlf {

class ExecutionInterface
{
public:
  ExecutionInterface()          = default;
  virtual ~ExecutionInterface() = default;

  using Name            = std::string;
  using SourceFiles     = fetch::vm::SourceFiles;
  using Target          = std::string;
  using Variant         = ExecutionResult::Variant;
  using PromiseOfResult = ExecutionResult::PromiseOfResult;
  using Params          = ExecutionParameters;

  virtual PromiseOfResult CreateExecutable(Target const &host, Name const &execName,
                                           SourceFiles const &sources)               = 0;
  virtual PromiseOfResult DeleteExecutable(Target const &host, Name const &execName) = 0;

  virtual PromiseOfResult CreateState(Target const &host, Name const &stateName) = 0;
  virtual PromiseOfResult CopyState(Target const &host, Name const &srcName,
                                    Name const &newName)                         = 0;
  virtual PromiseOfResult DeleteState(Target const &host, Name const &stateName) = 0;

  virtual PromiseOfResult Run(Target const &host, Name const &execName, Name const &stateName,
                              std::string const &entrypoint, Params const &params) = 0;

  ExecutionInterface(ExecutionInterface const &other) = delete;
  ExecutionInterface &operator=(ExecutionInterface const &other) = delete;
};

}  // namespace dmlf
}  // namespace fetch
