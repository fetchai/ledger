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

#include "dmlf/remote_execution_host.hpp"

namespace fetch {
namespace dmlf {

RemoteExecutionHost::Result RemoteExecutionHost::CreateExecutable(Name const & /*execName*/,
                                                                  SourceFiles const & /*sources*/)
{
  return 1234;
}
RemoteExecutionHost::Result RemoteExecutionHost::DeleteExecutable(Name const & /*execName*/)
{
  return 1234;
}

RemoteExecutionHost::Result RemoteExecutionHost::CreateState(Name const & /*stateName*/)
{
  return 1234;
}
RemoteExecutionHost::Result RemoteExecutionHost::CopyState(Name const & /*srcName*/,
                                                           Name const & /*newName*/)
{
  return 1234;
}
RemoteExecutionHost::Result RemoteExecutionHost::DeleteState(Name const & /*stateName*/)
{
  return 1234;
}

RemoteExecutionHost::Result RemoteExecutionHost::Run(Name const & /*execName*/,
                                                     Name const & /*stateName*/,
                                                     std::string const & /*entrypoint*/)
{
  return 1234;
}

}  // namespace dmlf
}  // namespace fetch
