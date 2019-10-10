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

#include "dmlf/execution_interface.hpp"

namespace fetch {
namespace dmlf {

class RemoteExecutionHost
{
public:
  using Result      = ExecutionInterface::Result;
  using Name        = ExecutionInterface::Name;
  using SourceFiles = ExecutionInterface::SourceFiles;

  RemoteExecutionHost()
  {}
  virtual ~RemoteExecutionHost()
  {}

  RemoteExecutionHost(RemoteExecutionHost const &other) = delete;
  RemoteExecutionHost &operator=(RemoteExecutionHost const &other)  = delete;
  bool                 operator==(RemoteExecutionHost const &other) = delete;
  bool                 operator<(RemoteExecutionHost const &other)  = delete;

  virtual Result CreateExecutable(Name const &execName, SourceFiles const &sources);
  virtual Result DeleteExecutable(Name const &execName);

  virtual Result CreateState(Name const &stateName);
  virtual Result CopyState(Name const &srcName, Name const &newName);
  virtual Result DeleteState(Name const &stateName);

  virtual Result Run(Name const &execName, Name const &stateName, std::string const &entrypoint);

protected:
private:
};

}  // namespace dmlf
}  // namespace fetch
