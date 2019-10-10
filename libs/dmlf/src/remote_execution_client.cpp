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

#include "dmlf/remote_execution_client.hpp"
#include "dmlf/remote_execution_protocol.hpp"

namespace fetch {
namespace dmlf {

RemoteExecutionClient::RemoteExecutionClient(RemoteExecutionClient::MuddlePtr    mud,
                                             std::shared_ptr<ExecutionInterface> local)
  : local_(local)
  , mud_(mud)
{
  auto client_ =
      std::make_shared<RpcClient>("Client", mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
}

RemoteExecutionClient::Returned RemoteExecutionClient::CreateExecutable(Target const &     target,
                                                                        Name const &       execName,
                                                                        SourceFiles const &sources)
{
  if (target == "" || target == "local:///")
  {
    return local_->CreateExecutable(target, execName, sources);
  }
  return Returned(client_->CallSpecificAddress(TargetUriToKey(target), RPC_DMLF,
                                               RemoteExecutionProtocol::RPC_DMLF_CREATE_EXE,
                                               execName, sources));
}

RemoteExecutionClient::Returned RemoteExecutionClient::DeleteExecutable(Target const &target,
                                                                        Name const &  execName)
{
  if (target == "" || target == "local:///")
  {
    return local_->DeleteExecutable(target, execName);
  }
  return Returned(client_->CallSpecificAddress(
      TargetUriToKey(target), RPC_DMLF, RemoteExecutionProtocol::RPC_DMLF_DEL_EXE, execName));
}

RemoteExecutionClient::Returned RemoteExecutionClient::CreateState(Target const &target,
                                                                   Name const &  stateName)
{
  if (target == "" || target == "local:///")
  {
    return local_->CreateState(target, stateName);
  }
  return Returned(client_->CallSpecificAddress(
      TargetUriToKey(target), RPC_DMLF, RemoteExecutionProtocol::RPC_DMLF_CREATE_STATE, stateName));
}

RemoteExecutionClient::Returned RemoteExecutionClient::CopyState(Target const &target,
                                                                 Name const &  srcName,
                                                                 Name const &  newName)
{
  if (target == "" || target == "local:///")
  {
    return local_->CopyState(target, srcName, newName);
  }
  return Returned(client_->CallSpecificAddress(TargetUriToKey(target), RPC_DMLF,
                                               RemoteExecutionProtocol::RPC_DMLF_COPY_STATE,
                                               srcName, newName));
}

RemoteExecutionClient::Returned RemoteExecutionClient::DeleteState(Target const &target,
                                                                   Name const &  stateName)
{
  if (target == "" || target == "local:///")
  {
    return local_->DeleteState(target, stateName);
  }
  return Returned(client_->CallSpecificAddress(
      TargetUriToKey(target), RPC_DMLF, RemoteExecutionProtocol::RPC_DMLF_DEL_STATE, stateName));
}

RemoteExecutionClient::Returned RemoteExecutionClient::Run(Target const &     target,
                                                           Name const &       execName,
                                                           Name const &       stateName,
                                                           std::string const &entrypoint)
{
  if (target == "" || target == "local:///")
  {
    return local_->Run(target, execName, stateName, entrypoint);
  }
  return Returned(client_->CallSpecificAddress(TargetUriToKey(target), RPC_DMLF,
                                               RemoteExecutionProtocol::RPC_DMLF_RUN, execName,
                                               stateName, entrypoint));
}

byte_array::ConstByteArray RemoteExecutionClient::TargetUriToKey(std::string const &target)
{
  Uri uri;
  uri.Parse(target);
  return fetch::byte_array::FromBase64(uri.GetMuddleAddress());
}

}  // namespace dmlf
}  // namespace fetch
