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
#include "dmlf/stochastic_reception_algorithm.hpp"
#include "muddle/rpc/client.hpp"
#include <cmath>  // for modf

namespace fetch {
namespace dmlf {
namespace colearn {

MuddleLearnerNetworkerImpl::MuddleLearnerNetworkerImpl(MuddlePtr mud, StorePtr update_store)
{
  Setup(std::move(mud), std::move(update_store));
}

void MuddleLearnerNetworkerImpl::SetShuffleAlgorithm(
    const std::shared_ptr<ShuffleAlgorithmInterface> &alg)
{
  ShuffleAlgorithmInterface *iface = alg.get();
  auto *                     stoc  = dynamic_cast<StochasticReceptionAlgorithm *>(iface);

  if (stoc != nullptr)
  {
    set_broadcast_proportion(stoc->broadcast_proportion());
  }
  else
  {
    alg_ = alg;
  }
}

void MuddleLearnerNetworkerImpl::Setup(MuddlePtr mud, StorePtr update_store)
{
  default_uri_.owner("default_owner").algorithm_class("default_algorithm").update_type("default_updatetype");
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

  subscription_ = mud_->GetEndpoint().Subscribe(SERVICE_DMLF, CHANNEL_COLEARN_BROADCAST);

  public_key_ = mud_->GetAddress();

  subscription_->SetMessageHandler([this](Address const &from, uint16_t service, uint16_t channel,
                                          uint16_t counter, Payload const &payload,
                                          Address const &my_address) {
    serializers::MsgPackSerializer buf{payload};

    std::string                uri_str;
    byte_array::ConstByteArray bytes;
    double                     proportion;
    double                     random_factor;

    auto source = std::string(fetch::byte_array::ToBase64(from));

    buf >> uri_str >> bytes >> proportion >> random_factor;

    std::cout << "BCAST from:" << source << ", "
              << "serv:" << service << ", "
              << "chan:" << channel << ", "
              << "cntr:" << counter << ", "
              << "addr:" << fetch::byte_array::ToBase64(my_address) << ", "
              << "uri :" << uri_str << ", "
              << "size:" << bytes.size() << " bytes, "
              << "prop:" << proportion << ", "
              << "fact:" << random_factor << ", " << std::endl;
    ;
    ProcessUpdate(uri_str, source, bytes, proportion, random_factor);
  });

  randomising_offset_   = randomiser_.GetNew();
  broadcast_proportion_ = 1.0;  // a reasonable default.
}

MuddleLearnerNetworkerImpl::Address MuddleLearnerNetworkerImpl::GetAddress() const
{
  return mud_->GetAddress();
}

std::string MuddleLearnerNetworkerImpl::GetAddressAsString() const
{
  return std::string(fetch::byte_array::ToBase64(mud_->GetAddress()));
}

MuddleLearnerNetworkerImpl::MuddleLearnerNetworkerImpl(const std::string &priv,
                                                       unsigned short int port,
                                                       const std::string &remote)
{
  std::unordered_set<std::string> remotes;
  if (!remote.empty())
  {
    remotes.insert(remote);
  }
  Setup(priv, port, remotes);
}

void MuddleLearnerNetworkerImpl::Setup(std::string const &priv, unsigned short int port,
                                       std::unordered_set<std::string> const &remotes)
{
  auto ident = std::make_shared<Signer>();
  if (priv.empty())
  {
    ident->GenerateKeys();
  }
  else
  {
    ident->Load(fetch::byte_array::FromBase64(priv));
  }

  netm_ = std::make_shared<NetMan>("LrnrNet", 4);
  netm_->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto mud = fetch::muddle::CreateMuddle("Test", ident, *netm_, "127.0.0.1");

  auto update_store = std::make_shared<UpdateStore>();

  mud->SetPeerSelectionMode(fetch::muddle::PeerSelectionMode::KADEMLIA);
  mud->Start(remotes, {port});
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  Setup(std::move(mud), std::move(update_store));
}

MuddleLearnerNetworkerImpl::MuddleLearnerNetworkerImpl(fetch::json::JSONDocument &cloud_config,
                                                       std::size_t                instance_number)
{
  auto my_config    = cloud_config.root()["peers"][instance_number];
  auto self_uri     = Uri(my_config["uri"].As<std::string>());
  auto port         = self_uri.GetTcpPeer().port();
  auto privkey      = my_config["key"].As<std::string>();
  auto config_peers = cloud_config.root()["peers"];

  auto config_peer_count = config_peers.size();

  std::unordered_set<std::string> remotes;

  if (config_peer_count <= INITIAL_PEERS_COUNT)
  {
    if (instance_number != 0)
    {
      remotes.insert(cloud_config.root()["peers"][0]["uri"].As<std::string>());
    }
  }
  else
  {
    for (unsigned int i = 0; i < INITIAL_PEERS_COUNT; i++)
    {
      auto peer_number = (instance_number + 1 + i) % config_peer_count;
      remotes.insert(config_peers[peer_number]["uri"].As<std::string>());
    }
  }

  Setup(privkey, port, remotes);
}

MuddleLearnerNetworkerImpl::~MuddleLearnerNetworkerImpl()
{
  taskpool_->stop();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  tasks_runners_->stop();
}

void MuddleLearnerNetworkerImpl::submit(TaskPtr const &t)
{
  taskpool_->submit(t);
}

void MuddleLearnerNetworkerImpl::PushUpdateBytes(ColearnURI const &uri_obj, Bytes const &update,
                                                 const Peers &peers)
{
  PushUpdateBytes(uri_obj, update, peers, broadcast_proportion_);
}

void MuddleLearnerNetworkerImpl::PushUpdateBytes(ColearnURI const &uri_obj, Bytes const &update,
                                                 const Peers &peers, double broadcast_proportion)
{
  auto random_factor   = randomiser_.GetNew();
  FETCH_LOG_INFO(LOGGING_NAME, "PushUpdateBytes(2) uri=", uri_obj.ToString());
  broadcast_proportion = std::max(0.0, std::min(1.0, broadcast_proportion));
  for (auto const &peer : peers)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Creating sender for ", uri_obj.ToString(), " to target ",
                   fetch::byte_array::ToBase64(peer));
    auto task = std::make_shared<MuddleOutboundUpdateTask>(peer, uri_obj.ToString(), update, client_,
                                                           broadcast_proportion, random_factor);
    taskpool_->submit(task);
  }
}

void MuddleLearnerNetworkerImpl::PushUpdateBytes(UpdateType const &type_name, Bytes const &update)
{
  ColearnURI uri_obj(default_uri_);
  FETCH_LOG_INFO(LOGGING_NAME, "PushUpdateBytes(3) uridef=", default_uri_.ToString());
  FETCH_LOG_INFO(LOGGING_NAME, "PushUpdateBytes(3) uriA=", uri_obj.ToString());
  uri_obj.update_type(type_name);
  FETCH_LOG_INFO(LOGGING_NAME, "PushUpdateBytes(3) uriB=", uri_obj.ToString());
  PushUpdateBytes(uri_obj, update);
}

void MuddleLearnerNetworkerImpl::PushUpdateBytes(ColearnURI const &uri_obj, Bytes const &update)
{
  auto random_factor = randomiser_.GetNew();
  if (alg_)
  {
    // use the shuffler
    auto next_ones = alg_->GetNextOutputs();

    Peers peers;
    for (auto const &next_one : next_ones)
    {
      auto id      = supplied_peers_[next_one];
      auto idbytes = fetch::byte_array::FromBase64(id);
      FETCH_LOG_INFO(LOGGING_NAME, "PushUpdateBytes, adding to sender list: ", next_one, " => ",
                     id);
      peers.insert(idbytes);
    }
    PushUpdateBytes(uri_obj, update, peers, 1.0);
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "PushUpdateBytes(4) uri=", uri_obj.ToString());
    serializers::MsgPackSerializer buf;
    buf << uri_obj.ToString() << update << broadcast_proportion_ << random_factor;
    mud_->GetEndpoint().Broadcast(SERVICE_DMLF, CHANNEL_COLEARN_BROADCAST, buf.data());
  }
}

MuddleLearnerNetworkerImpl::ConstUpdatePtr MuddleLearnerNetworkerImpl::GetUpdate(
    AlgorithmClass const &algo, UpdateType const &type, Criteria const &criteria)
{
  return update_store_->GetUpdate(algo, type, criteria);
}

uint64_t MuddleLearnerNetworkerImpl::ProcessUpdate(std::string const &uri_str,
                                                   std::string const &source,
                                                   byte_array::ConstByteArray update_bytes,
                                                   double proportion, double random_factor)
{
  double whole;
  if (std::modf(randomising_offset_ + random_factor, &whole) > proportion)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "DISCARDING ", uri_str,
                   " because ", randomising_offset_,
                   "+", random_factor, ">", proportion);
    return 0;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "STORING ", uri_str);

  UpdateStoreInterface::Metadata metadata;

  auto uri_obj = ColearnURI::Parse(uri_str);
  uri_obj.source(source);
  update_store_->PushUpdate(uri_obj, std::move(update_bytes), std::move(metadata));
  return 1;
}

uint64_t MuddleLearnerNetworkerImpl::NetworkColearnUpdate(service::CallContext const &context,
                                                          const std::string &uri_str,
                                                          byte_array::ConstByteArray update_bytes,
                                                          double proportion, double random_factor)
{
  auto source = std::string(fetch::byte_array::ToBase64(context.sender_address));
  return ProcessUpdate(uri_str, source, update_bytes, proportion, random_factor);
}

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
