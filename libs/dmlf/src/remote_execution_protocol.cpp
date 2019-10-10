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
#include "dmlf/remote_execution_protocol.hpp"

namespace fetch {
namespace dmlf {

RemoteExecutionProtocol::RemoteExecutionProtocol(RemoteExecutionHost &exec)
{
  Expose(RPC_DMLF_CREATE_EXE, &exec, &RemoteExecutionHost::CreateExecutable);
  Expose(RPC_DMLF_DEL_EXE, &exec, &RemoteExecutionHost::DeleteExecutable);
  Expose(RPC_DMLF_CREATE_STATE, &exec, &RemoteExecutionHost::CreateState);
  Expose(RPC_DMLF_COPY_STATE, &exec, &RemoteExecutionHost::CopyState);
  Expose(RPC_DMLF_DEL_STATE, &exec, &RemoteExecutionHost::DeleteState);
  Expose(RPC_DMLF_RUN, &exec, &RemoteExecutionHost::Run);
}

}  // namespace dmlf
}  // namespace fetch
