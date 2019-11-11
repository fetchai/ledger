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
#include "dmlf/colearn/colearn_protocol.hpp"
#include "dmlf/networkers/abstract_learner_networker.hpp"
#include "dmlf/update_interface.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "network/service/call_context.hpp"
#include "oef-base/threading/Threadpool.hpp"
#include "oef-base/threading/Taskpool.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

class MuddleLearnerNetworkerImpl : public dmlf::AbstractLearnerNetworker
{
public:
  using Taskpool           = oef::base::Taskpool;
  using Threadpool         = oef::base::Threadpool;
  using TaskP              = Taskpool::TaskP;
  using MuddlePtr          = muddle::MuddlePtr;
  using UpdateInterfacePtr = dmlf::UpdateInterfacePtr;
  using RpcClient          = fetch::muddle::rpc::Client;
  using RpcClientPtr       = std::shared_ptr<RpcClient>;
  using Proto              = ColearnProtocol;
  using ProtoP             = std::shared_ptr<ColearnProtocol>;
  using RpcServer          = fetch::muddle::rpc::Server;
  using RpcServerPtr       = std::shared_ptr<RpcServer>;
  using Bytes              = byte_array::ByteArray;


  explicit MuddleLearnerNetworkerImpl(MuddlePtr mud);
  ~MuddleLearnerNetworkerImpl() override;

  MuddleLearnerNetworkerImpl(MuddleLearnerNetworkerImpl const &other) = delete;
  MuddleLearnerNetworkerImpl &operator=(MuddleLearnerNetworkerImpl const &other)  = delete;
  bool                        operator==(MuddleLearnerNetworkerImpl const &other) = delete;
  bool                        operator<(MuddleLearnerNetworkerImpl const &other)  = delete;

  void addTarget(const std::string &peer);

  void PushUpdate(UpdateInterfacePtr const &update) override;
  void PushUpdateType(const std::string &type_name, UpdateInterfacePtr const &update) override;
  void PushUpdateBytes(const std::string &type_name, Bytes const &update);

  std::size_t GetPeerCount() const override
  {
    return 0;  // TODO(kll)
  }

  virtual void submit(TaskP const &t);

  // This is the exposed interface

  uint64_t NetworkColearnUpdate(service::CallContext const &context, const std::string &type_name,
                                const byte_array::ByteArray &bytes);

protected:
private:
  std::shared_ptr<Taskpool>   taskpool;
  std::shared_ptr<Threadpool> tasks_runners;
  MuddlePtr                   mud_;
  RpcClientPtr                client_;
  RpcServerPtr                server_;
  ProtoP                      proto_;

  std::unordered_set<std::string> peers_;

  friend class MuddleMessageHandler;
};

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
