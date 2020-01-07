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

#include "dmlf/execution/execution_interface.hpp"
#include "dmlf/execution/execution_result.hpp"

namespace fetch {
namespace dmlf {

class ExecutionEngineInterface
{
public:
  using Name        = ExecutionInterface::Name;
  using SourceFiles = ExecutionInterface::SourceFiles;
  using Target      = ExecutionInterface::Target;
  using Variant     = ExecutionInterface::Variant;
  using Params      = std::vector<Variant>;

  ExecutionEngineInterface()          = default;
  virtual ~ExecutionEngineInterface() = default;

  virtual ExecutionResult CreateExecutable(Name const &execName, SourceFiles const &sources) = 0;
  virtual ExecutionResult DeleteExecutable(Name const &execName)                             = 0;

  virtual ExecutionResult CreateState(Name const &stateName)                  = 0;
  virtual ExecutionResult CopyState(Name const &srcName, Name const &newName) = 0;
  virtual ExecutionResult DeleteState(Name const &stateName)                  = 0;

  virtual ExecutionResult Run(Name const &execName, Name const &stateName,
                              std::string const &entrypoint, Params params) = 0;
};

}  // namespace dmlf
}  // namespace fetch
