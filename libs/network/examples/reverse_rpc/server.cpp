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

#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "service_ids.hpp"

using fetch::muddle::Muddle;
using fetch::muddle::NetworkId;
using fetch::muddle::rpc::Server;
using fetch::muddle::rpc::Client;

const int SERVICE_TEST = 1;
const int CHANNEL_RPC  = 1;

#include <iostream>

#include <set>
using namespace fetch::service;
using namespace fetch::byte_array;

// First we make a service implementation
class ClientRegister
{
public:
  void Register(CallContext const *context)
  {
    std::cout << "\rRegistering " << ToBase64(context->sender_address) << std::endl
              << std::endl
              << "> " << std::flush;

    mutex_.lock();
    registered_aeas_.insert(context->sender_address);
    mutex_.unlock();
  }

  std::vector<std::string> SearchFor(std::string const &val)
  {
    std::vector<std::string> ret;
    mutex_.lock();
    for (auto &id : registered_aeas_)
    {
      auto prom =
          client_->CallSpecificAddress(id, FetchProtocols::NODE_TO_AEA, NodeToAEA::SEARCH, val);
      std::string s = prom->As<std::string>();
      if (s != "")
      {
        ret.push_back(s);
      }
    }

    mutex_.unlock();

    return ret;
  }

  void register_service_instance(std::shared_ptr<Muddle> muddle_ptr)
  {
    client_ = std::make_shared<Client>("RRPClient", muddle_ptr->AsEndpoint(), Muddle::Address(),
                                       SERVICE_TEST, CHANNEL_RPC);
  }

private:
  std::set<Muddle::Address> registered_aeas_;
  fetch::mutex::Mutex       mutex_{__LINE__, __FILE__};
  std::shared_ptr<Client>   client_;
};

// Next we make a protocol for the implementation
class AEAToNodeProtocol : public Protocol
{
public:
  AEAToNodeProtocol()
    : Protocol()
  {}

  void Expose(ClientRegister *target)
  {
    this->ExposeWithClientContext(AEAToNode::REGISTER, target, &ClientRegister::Register);
  }

private:
};

// And finanly we build the service
class OEFService
{
public:
  OEFService(std::shared_ptr<Muddle> &muddle)
  {
    auto server = std::make_shared<Server>(muddle->AsEndpoint(), SERVICE_TEST, CHANNEL_RPC);

    aea_to_node_protocol_.Expose(&client_register_);
    server->Add(FetchProtocols::AEA_TO_NODE, &aea_to_node_protocol_);
    client_register_.register_service_instance(muddle);
  }

  std::vector<std::string> SearchFor(std::string const &val)
  {
    return client_register_.SearchFor(val);
  }

private:
  AEAToNodeProtocol aea_to_node_protocol_;
  ClientRegister    client_register_;
};

int main()
{
  fetch::network::NetworkManager tm{"NetMgr", 8};

  auto server_muddle = Muddle::CreateMuddle(NetworkId{"TEST"}, tm);

  tm.Start();
  server_muddle->Start({8080});

  OEFService service(server_muddle);

  std::string search_for;
  std::cout << "Enter a string to search the AEAs for this string" << std::endl;
  while (true)
  {

    std::cout << "> " << std::flush;
    std::getline(std::cin, search_for);
    if ((search_for == "quit") || (!std::cin))
    {
      break;
    }

    auto results = service.SearchFor(search_for);
    for (auto &s : results)
    {
      std::cout << " - " << s << std::endl;
    }
  }

  tm.Stop();

  return 0;
}
