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

#include "core/byte_array/decoders.hpp"
#include "core/service_ids.hpp"
#include "dmlf/networkers/muddle_learner_networker.hpp"
#include "dmlf/update_interface.hpp"
#include "json/document.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"

namespace fetch {
namespace dmlf {

using std::chrono::seconds;
using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromBase64;
using fetch::network::NetworkManager;
using fetch::service::Promise;

using PromiseList = std::vector<Promise>;
using SignerPtr   = std::shared_ptr<crypto::ECDSASigner>;

MuddleLearnerNetworker::MuddleLearnerNetworkerProtocol::MuddleLearnerNetworkerProtocol(
    MuddleLearnerNetworker &sample)
{
  Expose(MuddleLearnerNetworkerProtocol::RECV_BYTES, &sample, &MuddleLearnerNetworker::RecvBytes);
}

MuddleLearnerNetworker::MuddleLearnerNetworker(const std::string &cloud_config,
                                               std::size_t        instance_number,
                                               const std::shared_ptr<NetworkManager> &netm,
                                               MuddleChannel                          channel_tmp)
  : channel_tmp_{channel_tmp}
{
  json::JSONDocument doc{cloud_config};

  NetworkConfigInit(doc, instance_number, netm);
}

MuddleLearnerNetworker::MuddleLearnerNetworker(fetch::json::JSONDocument &cloud_config,
                                               std::size_t                instance_number,
                                               const std::shared_ptr<NetworkManager> &netm,
                                               MuddleChannel                          channel_tmp)
  : channel_tmp_{channel_tmp}
{
  NetworkConfigInit(cloud_config, instance_number, netm);
}

// TOFIX remove return value
uint64_t MuddleLearnerNetworker::RecvBytes(const byte_array::ByteArray &b)
{
  switch (channel_tmp_)
  {
  case MuddleChannel::DEFAULT:
    NewMessage(b);
    break;
  case MuddleChannel::MULTIPLEX:
    NewDmlfMessage(b);
    break;
  default:;
  }
  return 0;
}

void MuddleLearnerNetworker::PushUpdate(UpdateInterfacePtr const &update)
{
  auto client =
      std::make_shared<RpcClient>("Client", mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
  auto data = update->Serialise();

  PromiseList promises;
  promises.reserve(20);

  for (auto const &target_peer : peers_)
  {
    promises.push_back(client->CallSpecificAddress(
        fetch::byte_array::FromBase64(byte_array::ConstByteArray(target_peer)), RPC_DMLF,
        MuddleLearnerNetworkerProtocol::RECV_BYTES, data));
  }

  for (auto &prom : promises)
  {
    prom->Wait();
  }
}

void MuddleLearnerNetworker::PushUpdateType(const std::string &       type,
                                            UpdateInterfacePtr const &update)
{
  auto client =
      std::make_shared<RpcClient>("Client", mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
  auto data = update->Serialise(type);

  PromiseList promises;
  promises.reserve(20);

  for (auto const &target_peer : peers_)
  {
    promises.push_back(client->CallSpecificAddress(
        fetch::byte_array::FromBase64(byte_array::ConstByteArray(target_peer)), RPC_DMLF,
        MuddleLearnerNetworkerProtocol::RECV_BYTES, data));
  }

  for (auto &prom : promises)
  {
    prom->Wait();
  }
}
std::size_t MuddleLearnerNetworker::GetPeerCount() const
{
  return peers_.size();
}

MuddleLearnerNetworker::CertificatePtr MuddleLearnerNetworker::CreateIdentity()
{
  SignerPtr certificate = std::make_shared<crypto::ECDSASigner>();
  certificate->GenerateKeys();
  return certificate;
}

MuddleLearnerNetworker::CertificatePtr MuddleLearnerNetworker::LoadIdentity(
    const std::string &privkey)
{
  using Signer = fetch::crypto::ECDSASigner;
  // load the key
  auto signer = std::make_unique<Signer>();
  signer->Load(FromBase64(privkey));

  return signer;
}

void MuddleLearnerNetworker::NetworkConfigInit(fetch::json::JSONDocument &doc,
                                               std::size_t                instance_number,
                                               const std::shared_ptr<NetworkManager> &netm)
{

  if (netm)
  {
    netm_ = netm;
  }
  else
  {
    netm_ = std::make_shared<NetworkManager>("LrnrNet", 16);
  }
  netm_->Start();

  auto my_config = doc.root()["peers"][instance_number];
  auto self_uri  = Uri(my_config["uri"].As<std::string>());
  auto port      = self_uri.GetTcpPeer().port();
  auto privkey   = my_config["key"].As<std::string>();

  ident_ = LoadIdentity(privkey);

  auto addr = self_uri.GetTcpPeer().address();

  mud_ = muddle::CreateMuddle("Test", ident_, *(this->netm_), addr);
  mud_->SetPeerSelectionMode(muddle::PeerSelectionMode::KADEMLIA);

  std::unordered_set<std::string> initial_peers;

  auto config_peers      = doc.root()["peers"];
  auto config_peer_count = config_peers.size();
  if (instance_number > 0)
  {
    initial_peers.insert(
        config_peers[(instance_number + 1) % config_peer_count]["uri"].As<std::string>());
  }

  mud_->Start(initial_peers, {port});

  server_ = std::make_shared<Server>(mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
  proto_  = std::make_shared<MuddleLearnerNetworkerProtocol>(*this);

  server_->Add(RPC_DMLF, proto_.get());

  for (std::size_t peer_number = 0; peer_number < config_peers.size(); peer_number++)
  {
    if (peer_number != instance_number)
    {
      peers_.emplace_back(config_peers[peer_number]["pub"].As<std::string>());
    }
  }
}

}  // namespace dmlf
}  // namespace fetch
