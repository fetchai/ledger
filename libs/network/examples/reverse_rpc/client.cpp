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
using fetch::network::NetworkManager;
using fetch::network::Uri;
using fetch::service::Protocol;
using fetch::mutex::Mutex;
using fetch::commandline::ParamsParser;
using std::chrono::milliseconds;
using std::this_thread::sleep_for;

static char const *LOGGING_NAME = "RPC-Client";

class AEA
{
public:
  Strings SearchFor(std::string const &val)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Searching for ", val);

    Strings ret{};
    {
      FETCH_LOCK(mutex_);

      for (auto const &s : strings_)
      {
        if (s.find(val) != std::string::npos)
        {
          ret.push_back(s);
        }
      }
    }

    return ret;
  }

  void AddString(std::string const &s)
  {
    FETCH_LOCK(mutex_);
    strings_.push_back(s);
  }

private:
  Mutex   mutex_{__LINE__, __FILE__};
  Strings strings_;
};

class AEAProtocol : public Protocol
{
public:
  explicit AEAProtocol(AEA *aea)
    : Protocol()
  {
    this->Expose(NodeToAEA::SEARCH, aea, &AEA::SearchFor);
  }
};

int main(int argc, char **argv)
{
  ParamsParser params;
  params.Parse(argc, argv);

  // create the AEA and associated protocol
  AEA         aea;
  AEAProtocol aea_protocol(&aea);
  for (std::size_t i = 1; i < params.arg_size(); ++i)
  {
    auto const item = params.GetArg(i);

    FETCH_LOG_INFO(LOGGING_NAME, "Registering item: ", item);
    aea.AddString(item);
  }

  // create and start the network manager
  NetworkManager tm{"NetMgr", 1};
  tm.Start();

  // create the muddle and attach all the RPC services
  auto muddle = Muddle::CreateMuddle(NetworkId{"TEST"}, tm);

  Client client{"Client", muddle->AsEndpoint(), Muddle::Address(), SERVICE_TEST, CHANNEL_RPC};

  // register the RPC server
  Server server{muddle->AsEndpoint(), SERVICE_TEST, CHANNEL_RPC};
  server.Add(FetchProtocols::NODE_TO_AEA, &aea_protocol);

  // start the muddle and wait for the connection to establish
  muddle->Start({}, {Uri{"tcp://127.0.0.1:8080"}});
  while (muddle->AsEndpoint().GetDirectlyConnectedPeers().empty())
  {
    sleep_for(milliseconds{100});
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Client Established Connection");

  // get the current target
  auto const target_address = muddle->AsEndpoint().GetDirectlyConnectedPeers()[0];

  // register this node as an AEA
  FETCH_LOG_INFO(LOGGING_NAME, "Registering node...");
  auto p =
      client.CallSpecificAddress(target_address, FetchProtocols::AEA_TO_NODE, AEAToNode::REGISTER);
  if (!p->Wait(1000, false))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Registering node...FAILED");
    return EXIT_FAILURE;
  }
  FETCH_LOG_INFO(LOGGING_NAME, "Registering node...complete");

  // run the main server loop while we have directly connected peers
  while (!muddle->AsEndpoint().GetDirectlyConnectedPeers().empty())
  {
    sleep_for(milliseconds{100});
  }

  return 0;
}
