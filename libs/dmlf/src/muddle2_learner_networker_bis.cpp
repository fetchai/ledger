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

#include "dmlf/muddle2_learner_networker_bis.hpp"
#include "dmlf/iupdate.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/json/document.hpp"

namespace fetch {
namespace dmlf {

using std::this_thread::sleep_for;
using std::chrono::seconds;
using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromBase64;
using fetch::network::NetworkManager;
using fetch::muddle::NetworkId;
using fetch::service::Promise;
using PromiseList = std::vector<Promise>;
using SignerPtr = std::shared_ptr<crypto::ECDSASigner>;
  
Muddle2LearnerNetworkerBis::Muddle2LearnerNetworkerBisProtocol::Muddle2LearnerNetworkerBisProtocol(Muddle2LearnerNetworkerBis &sample)
{
  Expose(1, &sample, &Muddle2LearnerNetworkerBis::RecvBytes);
}

Muddle2LearnerNetworkerBis::Muddle2LearnerNetworkerBis(const std::string cloud_config,
                                                 std::size_t instance_number,
                                                 std::shared_ptr<NetworkManager> netm)
{
  json::JSONDocument doc{cloud_config};


  FETCH_LOG_INFO("Muddle2LearnerNetworkerBis", "here 1");
  if (netm)
  {
    this -> netm = netm;
  }
  else
  {
    this -> netm = std::make_shared<NetworkManager>("NetMgrA", 4);
  }
  this -> netm -> Start();


  auto my_config = doc.root()["peers"][instance_number];
  auto self_uri = Uri(my_config["uri"].As<std::string>());
  auto port = self_uri.GetTcpPeer().port();
  auto privkey = my_config["key"].As<std::string>();

  //ident = CreateIdentity();
  ident = LoadIdentity(privkey);

  auto addr = self_uri.GetTcpPeer().address();

  mud = muddle::CreateMuddle("Test", ident, *(this -> netm), addr);
  mud -> SetPeerSelectionMode(muddle::PeerSelectionMode::KADEMLIA);

  std::unordered_set<std::string> initial_peers;
  if (instance_number > 0)
  {
    initial_peers.insert(doc.root()["peers"][0]["uri"].As<std::string>());
  }

  mud -> Start(initial_peers, {port});

  server = std::make_shared<Server>(mud -> GetEndpoint(), 1, 1);
  proto = std::make_shared<Muddle2LearnerNetworkerBisProtocol>(*this);

  server -> Add(1, proto.get());

  auto config_peers = doc.root()["peers"];
  
  for(std::size_t peer_number = 0; peer_number < config_peers.size(); peer_number++)
  {
    if (peer_number != instance_number)
    {
      peers.push_back(config_peers[peer_number]["pub"].As<std::string>());
    }
  }
}

#pragma clang diagnostic ignored "-Wunused-parameter"
uint64_t Muddle2LearnerNetworkerBis::RecvBytes(const byte_array::ByteArray &b)
{
  NewMessage(b);
  {
  Lock lock(mutex);
  updates.push_back(b);
  return updates.size();
  }
}

Muddle2LearnerNetworkerBis::~Muddle2LearnerNetworkerBis()
{
}

Muddle2LearnerNetworkerBis::Intermediate Muddle2LearnerNetworkerBis::getUpdateIntermediate()
{
  Lock lock(mutex);
  if (!updates.empty())
  {
    auto x = updates.front();
    updates.pop_front();
    return x;
  }
  throw std::length_error("Updates list is already empty.");
}

void Muddle2LearnerNetworkerBis::pushUpdate(std::shared_ptr<IUpdate> update)
{
  auto client = std::make_shared<RpcClient>("Client", mud -> GetEndpoint(), 1, 1);
  auto                                                data    = update->serialise();

  PromiseList promises;
  promises.reserve(20);
  
#pragma clang diagnostic ignored "-Wunused-variable"
  for (const auto &target_peer : peers)
  {
    promises.push_back(
      client->CallSpecificAddress(
        fetch::byte_array::FromBase64(
                                  byte_array::ConstByteArray(target_peer)
                                  )
        , 1, 1, data));
  }

  for (auto &prom : promises)
  {
    prom->Wait();
  }
}
/*
std::size_t Muddle2LearnerNetworkerBis::getUpdateCount() const
{
  return updates.size();
}
*/
std::size_t Muddle2LearnerNetworkerBis::getPeerCount() const
{
  return peers.size();
}

Muddle2LearnerNetworkerBis::CertificatePtr Muddle2LearnerNetworkerBis::CreateIdentity()
{
  SignerPtr certificate        = std::make_shared<crypto::ECDSASigner>();
  certificate->GenerateKeys();
  return certificate;
}

Muddle2LearnerNetworkerBis::CertificatePtr Muddle2LearnerNetworkerBis::LoadIdentity(const std::string &privkey)
{
  using Signer = fetch::crypto::ECDSASigner;
  // load the key
  auto signer = std::make_unique<Signer>();
  signer->Load(FromBase64(privkey));

  return signer;
}


}
}
