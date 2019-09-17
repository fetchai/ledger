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

#include "dmlf/muddle2_learner_networker.hpp"
#include "dmlf/iupdate.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "core/byte_array/decoders.hpp"

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

  Muddle2LearnerNetworker::Muddle2LearnerNetworkerProtocol::Muddle2LearnerNetworkerProtocol(Muddle2LearnerNetworker &sample)
  {
    Expose(1, &sample, &Muddle2LearnerNetworker::RecvBytes);
  }

Muddle2LearnerNetworker::Muddle2LearnerNetworker(unsigned short port,
                                                 const char *const identity,
                                                 std::unordered_set<std::string> tcp_peers,
                                                 std::shared_ptr<NetworkManager> netm )
{
  netm -> Start();
  this -> netm = netm;
  //TODO(kll) sotr address properly
  mud = muddle::CreateMuddle("Test", LoadIdentity(identity), *netm, "127.0.0.1");
  mud -> Start(std::move(tcp_peers), {port});

  server = std::make_shared<Server>(mud -> GetEndpoint(), 1, 1);
  proto = std::make_shared<Muddle2LearnerNetworkerProtocol>(*this);
  
  server -> Add(1, proto.get());
}

#pragma clang diagnostic ignored "-Wunused-parameter"
uint64_t Muddle2LearnerNetworker::RecvBytes(const byte_array::ByteArray &b)
{
  updates.push_back(b);
  return 125;
}

Muddle2LearnerNetworker::~Muddle2LearnerNetworker()
{
}

Muddle2LearnerNetworker::Intermediate Muddle2LearnerNetworker::getUpdateIntermediate()
{
    //TODO(kll)
  return Intermediate();
}

void Muddle2LearnerNetworker::pushUpdate(std::shared_ptr<IUpdate> update)
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
    FETCH_LOG_INFO("FUCK", "WAS ",prom->Wait());
  }
}
std::size_t Muddle2LearnerNetworker::getUpdateCount() const
{
  return updates.size();
}
std::size_t Muddle2LearnerNetworker::getPeerCount() const
{
  return peers.size();
}

void Muddle2LearnerNetworker::addPeers(Muddle2LearnerNetworker::Peers new_peers)
{
  for(const auto &new_peer : new_peers)
  {
    peers.push_back(new_peer);
  }
}


Muddle2LearnerNetworker::CertificatePtr Muddle2LearnerNetworker::LoadIdentity(char const *private_key)
{
  using Signer = fetch::crypto::ECDSASigner;

  // load the key
  auto signer = std::make_unique<Signer>();
  signer->Load(FromBase64(private_key));

  return signer;
}

}
}
