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
#include "service_consts.hpp"
#include <iostream>

#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"

using namespace fetch::service;
using namespace fetch::byte_array;

using fetch::muddle::Muddle;
using fetch::muddle::NetworkId;
using fetch::muddle::rpc::Server;
using fetch::muddle::rpc::Client;

const int SERVICE_TEST = 1;
const int CHANNEL_RPC  = 1;

// First we make a service implementation
class Implementation
{
public:
  int SlowFunction(int const &a, int const &b)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    return a + b;
  }

  int Add(int a, int b)
  {
    return a + b;
  }

  std::string Greet(std::string name)
  {
    return "Hello, " + name;
  }
};

// Next we make a protocol for the implementation
class ServiceProtocol : public Protocol
{
public:
  ServiceProtocol()
    : Protocol()
  {

    this->Expose(SLOWFUNCTION, &impl_, &Implementation::SlowFunction);
    this->Expose(ADD, &impl_, &Implementation::Add);
    this->Expose(GREET, &impl_, &Implementation::Greet);
  }

private:
  Implementation impl_;
};

int main()
{
  fetch::network::NetworkManager tm{"NetMgr", 8};
  auto server_muddle = Muddle::CreateMuddle(NetworkId{"TEST"}, tm);
  tm.Start();
  auto server = std::make_shared<Server>(server_muddle->AsEndpoint(), SERVICE_TEST, CHANNEL_RPC);
  server_muddle->Start({8080});

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;
  std::cin >> dummy;

  tm.Stop();
  server->Add(MYPROTO, new ServiceProtocol());

  return 0;
}
