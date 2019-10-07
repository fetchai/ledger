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

#include "core/byte_array/byte_array.hpp"
#include "dmlf/execution_result.hpp"
#include "network/generics/promise_of.hpp"
#include "vm/common.hpp"

#include <functional>
#include <ostream>
#include <string>
#include <vector>

namespace fetch {
namespace dmlf {

class ExecutionInterface
{
public:
  ExecutionInterface()          = default;
  virtual ~ExecutionInterface() = default;

  using Name        = std::string;
  using SourceFiles = fetch::vm::SourceFiles;
  using Target      = std::string;
  using Artifact    = fetch::vm::Variant;
  using Result      = ExecutionResult;
  using Returned    = fetch::network::PromiseOf<ExecutionResult>;
  //using Params      = std::vector<Artifact>;

  virtual Returned CreateExecutable(Target const &target, Name const &execName, SourceFiles const &sources) = 0;
  virtual Returned DeleteExecutable(Target const &target, Name const &execName) = 0;
  
  virtual Returned CreateState(Target const &target, Name const &stateName) = 0;
  virtual Returned CopyState(Target const &target, Name const &srcName, Name const &newName) = 0;
  virtual Returned DeleteState(Target const &target, Name const &stateName) = 0;
  
  virtual Returned Run(Target const &target, Name const &execName, Name const &stateName, std::string const &entrypoint) = 0;

  ExecutionInterface(ExecutionInterface const &other) = delete;
  ExecutionInterface &operator=(ExecutionInterface const &other) = delete;
  bool operator==(ExecutionInterface const &other) = delete;
  bool operator<(ExecutionInterface const &other) = delete;
protected:
private:
};

} // namespace dmlf
} // namespace fetch
