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

#include "network/service/server.hpp"

#include "core/threading/synchronised_state.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "service_ids.hpp"

#include <iostream>
#include <set>

using fetch::muddle::Muddle;
using fetch::muddle::NetworkId;
using fetch::muddle::rpc::Server;
using fetch::muddle::rpc::Client;
using fetch::service::CallContext;
using fetch::service::Protocol;

using MuddlePtr = std::shared_ptr<Muddle>;

static char const *LOGGING_NAME = "RPC-Server";

class ClientRegister
{
public:
  explicit ClientRegister(MuddlePtr muddle)
    : muddle_{std::move(muddle)}
  {}

  void Register(CallContext const context)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Registering: ", context.sender_address.ToBase64());

    node_set_.Apply([context](AddressSet &addresses) { addresses.insert(context.sender_address); });
  }

  Strings SearchFor(std::string const &val)
  {
    auto const connected_peers = muddle_->AsEndpoint().GetDirectlyConnectedPeers();

    // generate the set of address to whom we are directly connected and have registered
    AddressSet addresses{};
    node_set_.Apply([&addresses, &connected_peers](AddressSet const &node_addresses) {
      for (auto const &address : connected_peers)
      {
        if (node_addresses.find(address) != node_addresses.end())
        {
          addresses.insert(address);
        }
      }
    });

    // query all the connected addresses
    Strings strings{};
    for (auto const &address : addresses)
    {
      // query this specific address
      Strings response =
          client_->CallSpecificAddress(address, FetchProtocols::NODE_TO_AEA, NodeToAEA::SEARCH, val)
              ->As<Strings>();

      // if the node responded positively then add it to the response
      if (!response.empty())
      {
        strings.insert(strings.end(), response.begin(), response.end());
      }
    }

    return strings;
  }

private:
  using RpcClientPtr   = std::shared_ptr<Client>;
  using AddressSet     = std::unordered_set<Muddle::Address>;
  using SyncAddressSet = fetch::SynchronisedState<AddressSet>;

  MuddlePtr    muddle_;
  RpcClientPtr client_{
      std::make_shared<Client>("RRPClient", muddle_->AsEndpoint(), SERVICE_TEST, CHANNEL_RPC)};
  SyncAddressSet node_set_{};
};

// Next we make a protocol for the implementation
class AEAToNodeProtocol : public Protocol
{
public:
  explicit AEAToNodeProtocol(ClientRegister *target)
  {
    this->ExposeWithClientContext(AEAToNode::REGISTER, target, &ClientRegister::Register);
  }
};

// And finally we build the service
class OEFService
{
public:
  explicit OEFService(MuddlePtr const &muddle)
    : client_register_{muddle}
    , rpc_server_{std::make_shared<Server>(muddle->AsEndpoint(), SERVICE_TEST, CHANNEL_RPC)}
  {
    // add the protocol to the server
    rpc_server_->Add(FetchProtocols::AEA_TO_NODE, &aea_to_node_protocol_);
  }

  Strings SearchFor(std::string const &val)
  {
    return client_register_.SearchFor(val);
  }

private:
  using RpcServerPtr = std::shared_ptr<Server>;

  ClientRegister    client_register_;
  AEAToNodeProtocol aea_to_node_protocol_{&client_register_};
  RpcServerPtr      rpc_server_{};
};

int main()
{
  // create and start the network manager
  fetch::network::NetworkManager tm{"NetMgr", 8};
  tm.Start();

  // create and start the muddle service
  auto muddle = Muddle::CreateMuddle(NetworkId{"TEST"}, tm);
  muddle->Start({8080});

  // create the OEF service and attach it to the muddle
  OEFService service(muddle);

  std::string search_for;
  std::cout << "Enter a string to search the AEAs for this string" << std::endl;
  for (;;)
  {

    std::cout << "> " << std::flush;
    std::getline(std::cin, search_for);
    if ((search_for == "quit") || (!std::cin))
    {
      break;
    }

    // a blank search is not searched
    if (search_for.empty())
    {
      continue;
    }

    auto results = service.SearchFor(search_for);
    for (auto &s : results)
    {
      std::cout << " - " << s << std::endl;
    }
  }

  return 0;
}
