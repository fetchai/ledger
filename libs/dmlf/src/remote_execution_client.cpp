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

#include "core/byte_array/decoders.hpp"
#include "core/service_ids.hpp"
#include "dmlf/remote_execution_client.hpp"
#include "dmlf/remote_execution_protocol.hpp"

namespace fetch {
namespace dmlf {

using fetch::byte_array::FromBase64;

RemoteExecutionClient::RemoteExecutionClient(RemoteExecutionClient::MuddlePtr    mud,
                                             std::shared_ptr<ExecutionInterface> local)
  : local_(std::move(local))
  , mud_(std::move(mud))
{
  client_ = std::make_shared<RpcClient>("Client", mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
}

RemoteExecutionClient::PromiseOfResult RemoteExecutionClient::CreateExecutable(
    Target const &target, Name const &execName, SourceFiles const &sources)
{
  if (target.empty() || target == LOCAL)
  {
    return local_->CreateExecutable(target, execName, sources);
  }
  return Returned([this, target, execName, sources](OpIdent const &op_id) {
    client_->CallSpecificAddress(TargetUriToKey(target), RPC_DMLF,
                                 RemoteExecutionProtocol::RPC_DMLF_CREATE_EXE, op_id, execName,
                                 sources);
    return true;
  });
}

RemoteExecutionClient::PromiseOfResult RemoteExecutionClient::DeleteExecutable(Target const &target,
                                                                               Name const &execName)
{
  if (target.empty() || target == LOCAL)
  {
    return local_->DeleteExecutable(target, execName);
  }
  return Returned([this, target, execName](OpIdent const &op_id) {
    client_->CallSpecificAddress(TargetUriToKey(target), RPC_DMLF,

                                 RemoteExecutionProtocol::RPC_DMLF_DEL_EXE, op_id, execName);
    return true;
  });
}

RemoteExecutionClient::PromiseOfResult RemoteExecutionClient::CreateState(Target const &target,
                                                                          Name const &  stateName)
{
  if (target.empty() || target == LOCAL)
  {
    return local_->CreateState(target, stateName);
  }
  return Returned([this, target, stateName](OpIdent const &op_id) {
    client_->CallSpecificAddress(TargetUriToKey(target), RPC_DMLF,
                                 RemoteExecutionProtocol::RPC_DMLF_CREATE_STATE, op_id, stateName);
    return true;
  });
}

RemoteExecutionClient::PromiseOfResult RemoteExecutionClient::CopyState(Target const &target,
                                                                        Name const &  srcName,
                                                                        Name const &  newName)
{
  if (target.empty() || target == LOCAL)
  {
    return local_->CopyState(target, srcName, newName);
  }
  return Returned([this, target, srcName, newName](OpIdent const &op_id) {
    client_->CallSpecificAddress(TargetUriToKey(target), RPC_DMLF,
                                 RemoteExecutionProtocol::RPC_DMLF_COPY_STATE, op_id, srcName,
                                 newName);
    return true;
  });
}

RemoteExecutionClient::PromiseOfResult RemoteExecutionClient::DeleteState(Target const &target,
                                                                          Name const &  stateName)
{
  if (target.empty() || target == LOCAL)
  {
    return local_->DeleteState(target, stateName);
  }
  return Returned([this, target, stateName](OpIdent const &op_id) {
    client_->CallSpecificAddress(TargetUriToKey(target), RPC_DMLF,
                                 RemoteExecutionProtocol::RPC_DMLF_DEL_STATE, op_id, stateName);
    return true;
  });
}

RemoteExecutionClient::PromiseOfResult RemoteExecutionClient::Run(Target const &     target,
                                                                  Name const &       execName,
                                                                  Name const &       stateName,
                                                                  std::string const &entrypoint,
                                                                  Params const &     params)
{
  if (target.empty() || target == LOCAL)
  {
    return local_->Run(target, execName, stateName, entrypoint, {});
  }
  return Returned([this, target, execName, stateName, entrypoint, params](OpIdent const &op_id) {
    client_->CallSpecificAddress(TargetUriToKey(target), RPC_DMLF,
                                 RemoteExecutionProtocol::RPC_DMLF_RUN, op_id, execName, stateName,
                                 entrypoint, params);
    return true;
  });
}

byte_array::ConstByteArray RemoteExecutionClient::TargetUriToKey(std::string const &target)
{
  return fetch::byte_array::FromBase64(target);
}

bool RemoteExecutionClient::ReturnResults(OpIdent const &op_id, ExecutionResult const &result)
{
  auto iter = pending_results_.find(op_id);
  if (iter == pending_results_.end())
  {
    // TODO(kll) report some sort of error here.
  }
  else
  {
    serializers::MsgPackSerializer serializer;
    serializer << result;
    iter->second.GetInnerPromise()->Fulfill(serializer.data());
  }
  return true;
}

RemoteExecutionClient::PromiseOfResult RemoteExecutionClient::Returned(
    const std::function<bool(OpIdent const &op_id)> &func)
{
  counter++;
  auto op_id = std::to_string(counter);
  func(op_id);
  auto prom               = service::MakePromise();
  auto prom_of            = fetch::network::PromiseOf<ExecutionResult>(prom);
  pending_results_[op_id] = prom_of;
  return prom_of;
}

}  // namespace dmlf
}  // namespace fetch
