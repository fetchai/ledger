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

#define FETCH_DISABLE_LOGGING
#include "core/commandline/parameter_parser.hpp"
#include "core/logger.hpp"
#include "core/serializers/byte_array.hpp"
#include "network/service/service_client.hpp"

#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "service_ids.hpp"

using fetch::muddle::Muddle;
using fetch::muddle::rpc::Server;
using fetch::muddle::rpc::Client;
using fetch::muddle::NetworkId;

#include <iostream>

using namespace fetch::commandline;
using namespace fetch::service;
using namespace fetch::byte_array;

const int SERVICE_TEST = 1;
const int CHANNEL_RPC  = 1;

class AEA
{
public:
  std::string SearchFor(std::string val)
  {
    std::cout << "Searching for " << val << std::endl;

    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    std::string                          ret = "";

    for (auto &s : strings_)
    {
      if (s.find(val) != std::string::npos)
      {
        ret = s;
        break;
      }
    }

    return ret;
  }

  void AddString(std::string const &s)
  {
    strings_.push_back(s);
  }

private:
  std::vector<std::string> strings_;

  fetch::mutex::Mutex mutex_{__LINE__, __FILE__};
};

class AEAProtocol : public Protocol
{
public:
  AEAProtocol(AEA *aea)
    : Protocol()
  {
    this->Expose(NodeToAEA::SEARCH, aea, &AEA::SearchFor);
  }

private:
};

int main(int argc, char **argv)
{
  ParamsParser params;
  params.Parse(argc, argv);

  // Client setup
  fetch::network::NetworkManager tm{"NetMgr", 1};

  auto                client_muddle = Muddle::CreateMuddle(NetworkId{"TEST"}, tm);
  fetch::network::Uri peer("tcp://127.0.0.1:8080");
  client_muddle->AddPeer(peer);
  auto client = std::make_shared<Client>("Client", client_muddle->AsEndpoint(), Muddle::Address(),
                                         SERVICE_TEST, CHANNEL_RPC);
  auto server = std::make_shared<Server>(client_muddle->AsEndpoint(), SERVICE_TEST, CHANNEL_RPC);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  Muddle::Address target_address;
  if (!client_muddle->UriToDirectAddress(peer, target_address))
  {
    std::cout << "Can't connect" << std::endl;
    exit(1);
  }

  AEA         aea;
  AEAProtocol aea_protocol(&aea);
  for (std::size_t i = 0; i < params.arg_size(); ++i)
  {
    aea.AddString(params.GetArg(i));
  }

  tm.Start();
  client_muddle->Start({});

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  server->Add(FetchProtocols::NODE_TO_AEA, &aea_protocol);

  auto p =
      client->CallSpecificAddress(target_address, FetchProtocols::AEA_TO_NODE, AEAToNode::REGISTER);

  FETCH_LOG_PROMISE();
  if (p->Wait())
  {
    std::cout << "Node registered" << std::endl;

    for (;;)
    {
    }
  }

  tm.Stop();

  return 0;
}
