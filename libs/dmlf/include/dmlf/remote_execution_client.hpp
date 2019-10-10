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

#include "core/service_ids.hpp"
#include "dmlf/execution_interface.hpp"
#include "dmlf/vm_wrapper_interface.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"

namespace fetch {
namespace dmlf {

class RemoteExecutionClient : public ExecutionInterface
{
public:
  using MuddlePtr = muddle::MuddlePtr;
  using Uri       = network::Uri;
  using RpcClient = fetch::muddle::rpc::Client;

  RemoteExecutionClient(MuddlePtr mud_, std::shared_ptr<ExecutionInterface> local =
                                            std::shared_ptr<ExecutionInterface>());

  virtual ~RemoteExecutionClient() = default;

  static constexpr char const *LOCAL = "local:///";

  RemoteExecutionClient(RemoteExecutionClient const &other) = delete;
  RemoteExecutionClient &operator=(RemoteExecutionClient const &other)  = delete;
  bool                   operator==(RemoteExecutionClient const &other) = delete;
  bool                   operator<(RemoteExecutionClient const &other)  = delete;

  Returned CreateExecutable(Target const &target, Name const &execName,
                            SourceFiles const &sources) override;
  Returned DeleteExecutable(Target const &target, Name const &execName) override;

  Returned CreateState(Target const &target, Name const &stateName) override;
  Returned CopyState(Target const &target, Name const &srcName, Name const &newName) override;
  Returned DeleteState(Target const &target, Name const &stateName) override;

  Returned Run(Target const &target, Name const &execName, Name const &stateName,
               std::string const &entrypoint) override;

protected:
private:
  using CertificatePtr = muddle::ProverPtr;

  std::shared_ptr<ExecutionInterface> local_;
  MuddlePtr                           mud_;
  std::shared_ptr<RpcClient>          client_;

  byte_array::ConstByteArray TargetUriToKey(std::string const &target);
};

}  // namespace dmlf
}  // namespace fetch
