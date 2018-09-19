//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
using namespace fetch::service;
using namespace fetch::byte_array;

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

// And finanly we build the service
class MyCoolService : public ServiceServer<fetch::network::TCPServer>
{
public:
  MyCoolService(uint16_t port, fetch::network::NetworkManager tm)
    : ServiceServer(port, tm)
  {
    this->Add(MYPROTO, new ServiceProtocol());
  }
};

int main()
{
  fetch::network::NetworkManager tm(8);
  MyCoolService                  serv(8080, tm);
  tm.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;
  std::cin >> dummy;

  tm.Stop();

  return 0;
}
