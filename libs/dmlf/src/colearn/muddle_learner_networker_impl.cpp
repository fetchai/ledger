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

#include "dmlf/colearn/muddle_learner_networker_impl.hpp"
#include "dmlf/colearn/muddle_outbound_update_task.hpp"
#include "muddle/rpc/client.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

void MuddleLearnerNetworkerImpl::addTarget(const std::string &peer)
{
  peers_.insert(peer);
}

MuddleLearnerNetworkerImpl::MuddleLearnerNetworkerImpl(MuddlePtr mud)
  : mud_(std::move(mud))
{
  taskpool      = std::make_shared<Taskpool>();
  tasks_runners = std::make_shared<Threadpool>();
  std::function<void(std::size_t thread_number)> run_tasks =
      std::bind(&Taskpool::run, taskpool.get(), std::placeholders::_1);
  tasks_runners->start(5, run_tasks);

  client_ = std::make_shared<RpcClient>("Client", mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
  proto_  = std::make_shared<ColearnProtocol>(*this);
  server_ = std::make_shared<RpcServer>(mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
  server_->Add(RPC_COLEARN, proto_.get());
}

MuddleLearnerNetworkerImpl::~MuddleLearnerNetworkerImpl()
{
  taskpool->stop();
  ::usleep(10000);
  tasks_runners->stop();
}

void MuddleLearnerNetworkerImpl::submit(const TaskP &t)
{
  taskpool->submit(t);
}

void MuddleLearnerNetworkerImpl::PushUpdateType(const std::string &       type_name,
                                                const UpdateInterfacePtr &update)
{
  std::cout << "UPDATE(" << type_name << std::endl;
  auto bytes = update->Serialise();
  PushUpdateBytes(type_name, bytes);
}
void MuddleLearnerNetworkerImpl::PushUpdateBytes(const std::string &type_name, const Bytes &update)
{
  std::cout << "UPDATE BYTES(" << type_name << std::endl;
  for (const auto &peer : peers_)
  {
    std::cout << "TASKING SEND (" << type_name << ")TO:" << peer << std::endl;
    auto task = std::make_shared<MuddleOutboundUpdateTask>(peer, type_name, update, client_);
    taskpool->submit(task);
  }
}

void MuddleLearnerNetworkerImpl::PushUpdate(const UpdateInterfacePtr &update)
{
  PushUpdateType("", update);
}

uint64_t MuddleLearnerNetworkerImpl::NetworkColearnUpdate(service::CallContext const & context,
                                                          const std::string &          type_name,
                                                          const byte_array::ByteArray &bytes)
{
  std::cout << "NetworkColearnUpdate from " << context.sender_address << std::endl;
  NewMessage(type_name, bytes);
  return 1;
}
}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
