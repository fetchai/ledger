//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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
#include "service_consts.hpp"
#include <iostream>

#include <set>
using namespace fetch::service;
using namespace fetch::byte_array;

// First we make a service implementation
class ClientRegister
{
public:
  void Register(uint64_t client)
  {
    std::cout << "\rRegistering " << client << std::endl << std::endl << "> " << std::flush;

    mutex_.lock();
    registered_aeas_.insert(client);
    mutex_.unlock();
  }

  std::vector<std::string> SearchFor(std::string const &val)
  {
    detailed_assert(service_ != nullptr);

    std::vector<std::string> ret;
    mutex_.lock();
    for (auto &id : registered_aeas_)
    {
      auto &rpc = service_->ServiceInterfaceOf(id);

      std::string s =
          rpc.Call(FetchProtocols::NODE_TO_AEA, NodeToAEA::SEARCH, val).As<std::string>();
      if (s != "")
      {
        ret.push_back(s);
      }
    }

    mutex_.unlock();

    return ret;
  }

  void register_service_instance(ServiceServer<fetch::network::TCPServer> *ptr) { service_ = ptr; }

private:
  ServiceServer<fetch::network::TCPServer> *service_ = nullptr;
  std::set<uint64_t>                        registered_aeas_;
  fetch::mutex::Mutex                       mutex_;
};

// Next we make a protocol for the implementation
class AEAToNodeProtocol : public ClientRegister, public Protocol
{
public:
  AEAToNodeProtocol() : ClientRegister(), Protocol()
  {
    this->ExposeWithClientArg(AEAToNode::REGISTER, (ClientRegister *)this,
                              &ClientRegister::Register);
  }

private:
};

// And finanly we build the service
class OEFService : public ServiceServer<fetch::network::TCPServer>
{
public:
  OEFService(uint16_t port, fetch::network::NetworkManager tm) : ServiceServer(port, tm)
  {
    this->Add(FetchProtocols::AEA_TO_NODE, &aea_to_node_);
    aea_to_node_.register_service_instance(this);
  }

  std::vector<std::string> SearchFor(std::string const &val) { return aea_to_node_.SearchFor(val); }

private:
  AEAToNodeProtocol aea_to_node_;
};

int main()
{
  fetch::network::NetworkManager tm(8);
  OEFService                     serv(8080, tm);
  tm.Start();

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

    auto results = serv.SearchFor(search_for);
    for (auto &s : results)
    {
      std::cout << " - " << s << std::endl;
    }
  }

  tm.Stop();

  return 0;
}
