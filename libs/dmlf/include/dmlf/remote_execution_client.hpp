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
#include "dmlf/execution/execution_params.hpp"
#include "dmlf/execution_workload.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"

namespace fetch {
namespace dmlf {

class RemoteExecutionClient : public ExecutionInterface
{
public:
  using MuddlePtr       = muddle::MuddlePtr;
  using Uri             = network::Uri;
  using RpcClient       = fetch::muddle::rpc::Client;
  using PromiseOfResult = fetch::network::PromiseOf<ExecutionResult>;
  using OpIdent         = ExecutionWorkload::OpIdent;
  using PendingResults  = std::map<OpIdent, PromiseOfResult>;
  using Params          = ExecutionParameters;

  explicit RemoteExecutionClient(MuddlePtr mud, std::shared_ptr<ExecutionInterface> local =
                                                    std::shared_ptr<ExecutionInterface>());

  ~RemoteExecutionClient() override = default;

  static constexpr char const *LOCAL = "local:///";

  RemoteExecutionClient(RemoteExecutionClient const &other) = delete;
  RemoteExecutionClient &operator=(RemoteExecutionClient const &other)  = delete;
  bool                   operator==(RemoteExecutionClient const &other) = delete;
  bool                   operator<(RemoteExecutionClient const &other)  = delete;

  PromiseOfResult CreateExecutable(Target const &target, Name const &execName,
                                   SourceFiles const &sources) override;
  PromiseOfResult DeleteExecutable(Target const &target, Name const &execName) override;

  PromiseOfResult CreateState(Target const &target, Name const &stateName) override;
  PromiseOfResult CopyState(Target const &target, Name const &srcName,
                            Name const &newName) override;
  PromiseOfResult DeleteState(Target const &target, Name const &stateName) override;

  PromiseOfResult Run(Target const &target, Name const &execName, Name const &stateName,
                      std::string const &entrypoint, Params const &params) override;

  // This is the exported interface which is called with results from the remote host.

  bool ReturnResults(OpIdent const &op_id, ExecutionResult const &result);

protected:
private:
  std::shared_ptr<ExecutionInterface> local_;
  MuddlePtr                           mud_;
  std::shared_ptr<RpcClient>          client_;

  PendingResults pending_results_;
  std::size_t    counter = 0;

  PromiseOfResult Returned(const std::function<bool(OpIdent const &op_id)> &func);

  byte_array::ConstByteArray TargetUriToKey(std::string const &target);
};

}  // namespace dmlf
}  // namespace fetch
