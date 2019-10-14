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

#include "core/service_ids.hpp"
#include "dmlf/remote_execution_host.hpp"
#include "dmlf/remote_execution_protocol.hpp"
#include "dmlf/execution/execution_result.hpp"

namespace fetch {
namespace dmlf {

  RemoteExecutionHost::RemoteExecutionHost(MuddlePtr mud, ExecutionInterfacePtr executor)
    : mud_(mud)
    , executor_(executor)
  {
    client_ = std::make_shared<RpcClient>("Host", mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
  }

  bool RemoteExecutionHost::CreateExecutable(service::CallContext const &context,
                                             OpIdent const &op_id, Name const &execName,
                                             SourceFiles const &sources)
  {
    pending_workloads_ . emplace_back(ExecutionWorkload(context.sender_address, op_id, "",
                                                        [execName, sources](ExecutionInterfacePtr const &exec){
                                                          return exec->CreateExecutable(execName, sources);
                                                        }));
    return true;
  }
  bool RemoteExecutionHost::DeleteExecutable(service::CallContext const & context,
                                             OpIdent const & op_id, Name const & execName)
  {
    pending_workloads_ . emplace_back(ExecutionWorkload(context.sender_address, op_id, "",
                                                        [execName](ExecutionInterfacePtr const &exec){
                                                          return exec->DeleteExecutable(execName);
                                                        }));
    return true;
  }

  bool RemoteExecutionHost::CreateState(service::CallContext const & context,
                                        OpIdent const & op_id, Name const & stateName)
  {
    pending_workloads_ . emplace_back(ExecutionWorkload(context.sender_address, op_id, "",
                                                        [stateName](ExecutionInterfacePtr const &exec){
                                                          return exec->CreateState(stateName);
                                                        }));  return true;
  }
  bool RemoteExecutionHost::CopyState(service::CallContext const & context,
                                      OpIdent const & op_id, Name const & srcName,
                                      Name const & newName)
  {
    pending_workloads_ . emplace_back(ExecutionWorkload(context.sender_address, op_id, "",
                                                        [srcName, newName](ExecutionInterfacePtr const &exec){
                                                          return exec->CopyState(srcName, newName);
                                                        }));  return true;
  }
  bool RemoteExecutionHost::DeleteState(service::CallContext const & context,
                                        OpIdent const & op_id, Name const & stateName)
  {
    pending_workloads_ . emplace_back(ExecutionWorkload(context.sender_address, op_id, "",
                                                        [stateName](ExecutionInterfacePtr const &exec){
                                                          return exec->DeleteState(stateName);
                                                        }));  return true;
  }

  bool RemoteExecutionHost::Run(service::CallContext const & context,
                                OpIdent const & op_id, Name const & execName,
                                Name const & stateName, std::string const & entrypoint)
  {
    pending_workloads_ . emplace_back(ExecutionWorkload(context.sender_address, op_id, "",
                                                        [execName, stateName, entrypoint](ExecutionInterfacePtr const &exec){
                                                          return exec->Run(execName, stateName, entrypoint);
                                                        }));
    return true;
  }

  bool RemoteExecutionHost::ExecuteOneWorkload(void)
{
  if (pending_workloads_.empty())
  {
    return false;
  }

  auto wl = pending_workloads_.front();
  pending_workloads_.pop_front();
  auto res = wl.worker_(executor_);

  client_->CallSpecificAddress(wl.respondent_, RPC_DMLF, RemoteExecutionProtocol::RPC_DMLF_RESULTS,
                               wl.op_id_, res);

  return true;
}

}  // namespace dmlf
}  // namespace fetch
