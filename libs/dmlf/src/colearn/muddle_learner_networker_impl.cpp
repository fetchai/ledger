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

#include "crypto/ecdsa.hpp"
#include "dmlf/colearn/muddle_learner_networker_impl.hpp"
#include "dmlf/colearn/muddle_outbound_update_task.hpp"
#include "dmlf/colearn/update_store.hpp"
#include "muddle/rpc/client.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

void MuddleLearnerNetworkerImpl::addTarget(const std::string &peer)
{
  peers_.insert(peer);
}

MuddleLearnerNetworkerImpl::MuddleLearnerNetworkerImpl(MuddlePtr mud, StorePtr update_store)
{
  setup(mud, update_store);
}

void MuddleLearnerNetworkerImpl::setup(MuddlePtr mud, StorePtr update_store)
{
  mud_           = std::move(mud);
  update_store_  = std::move(update_store);
  taskpool_      = std::make_shared<Taskpool>();
  tasks_runners_ = std::make_shared<Threadpool>();
  std::function<void(std::size_t thread_number)> run_tasks =
      std::bind(&Taskpool::run, taskpool_.get(), std::placeholders::_1);
  tasks_runners_->start(5, run_tasks);

  client_ = std::make_shared<RpcClient>("Client", mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
  proto_  = std::make_shared<ColearnProtocol>(*this);
  server_ = std::make_shared<RpcServer>(mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
  server_->Add(RPC_COLEARN, proto_.get());
}

MuddleLearnerNetworkerImpl::MuddleLearnerNetworkerImpl(const std::string &priv,
                                                       unsigned short int port,
                                                       const std::string &remote)
{
  auto signer = std::make_shared<Signer>();
  signer->Load(fetch::byte_array::FromBase64(priv));
  CertificatePtr ident = signer;

  netm_ = std::make_shared<NetMan>("LrnrNet", 4);
  netm_->Start();
  auto mud = fetch::muddle::CreateMuddle("Test", ident, *netm_, "127.0.0.1");

  auto update_store = std::make_shared<UpdateStore>();

  std::unordered_set<std::string> remotes;
  if (remote.empty())
  {
    remotes.insert(remote);
  }

  mud->SetPeerSelectionMode(fetch::muddle::PeerSelectionMode::KADEMLIA);
  mud->Start(remotes, {port});

  setup(std::move(mud), std::move(update_store));
}

MuddleLearnerNetworkerImpl::~MuddleLearnerNetworkerImpl()
{
  taskpool_->stop();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  tasks_runners_->stop();
}

void MuddleLearnerNetworkerImpl::submit(TaskP const &t)
{
  taskpool_->submit(t);
}

void MuddleLearnerNetworkerImpl::PushUpdateType(const std::string &       type_name,
                                                UpdateInterfacePtr const &update)
{
  auto bytes = update->Serialise();
  PushUpdateBytes(type_name, bytes);
}
void MuddleLearnerNetworkerImpl::PushUpdateBytes(const std::string &type_name, Bytes const &update)
{
  for (auto const &peer : peers_)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Creating sender for ", type_name, " to target ", peer);
    auto task = std::make_shared<MuddleOutboundUpdateTask>(peer, type_name, update, client_);
    taskpool_->submit(task);
  }
}

MuddleLearnerNetworkerImpl::UpdatePtr MuddleLearnerNetworkerImpl::GetUpdate(
    Algorithm const &algo, UpdateType const &type, Criteria const &criteria)
{
  return update_store_->GetUpdate(algo, type, criteria);
}

void MuddleLearnerNetworkerImpl::PushUpdate(UpdateInterfacePtr const &update)
{
  PushUpdateType("", update);
}

uint64_t MuddleLearnerNetworkerImpl::NetworkColearnUpdate(service::CallContext const &context,
                                                          const std::string &         type_name,
                                                          byte_array::ConstByteArray  bytes)
{
  auto source = std::string(fetch::byte_array::ToBase64(context.sender_address));
  FETCH_LOG_INFO(LOGGING_NAME, "Update for ", type_name, " from ", source);
  UpdateStoreInterface::Metadata metadata;
  update_store_->PushUpdate("algo1", type_name, std::move(bytes), source, std::move(metadata));
  return 1;
}
}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
