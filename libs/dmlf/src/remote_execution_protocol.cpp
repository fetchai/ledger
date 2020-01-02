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

#include "dmlf/remote_execution_client.hpp"
#include "dmlf/remote_execution_host.hpp"
#include "dmlf/remote_execution_protocol.hpp"

namespace fetch {
namespace dmlf {

RemoteExecutionProtocol::RemoteExecutionProtocol(RemoteExecutionHost &exec)
{
  ExposeWithClientContext(RPC_DMLF_CREATE_EXE, &exec, &RemoteExecutionHost::CreateExecutable);
  ExposeWithClientContext(RPC_DMLF_DEL_EXE, &exec, &RemoteExecutionHost::DeleteExecutable);
  ExposeWithClientContext(RPC_DMLF_CREATE_STATE, &exec, &RemoteExecutionHost::CreateState);
  ExposeWithClientContext(RPC_DMLF_COPY_STATE, &exec, &RemoteExecutionHost::CopyState);
  ExposeWithClientContext(RPC_DMLF_DEL_STATE, &exec, &RemoteExecutionHost::DeleteState);
  ExposeWithClientContext(RPC_DMLF_RUN, &exec, &RemoteExecutionHost::Run);
}

RemoteExecutionProtocol::RemoteExecutionProtocol(RemoteExecutionClient &exec)
{
  Expose(RPC_DMLF_RESULTS, &exec, &RemoteExecutionClient::ReturnResults);
}

}  // namespace dmlf
}  // namespace fetch
