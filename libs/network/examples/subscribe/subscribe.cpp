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

#include "network/management/network_manager.hpp"
#include "subscribe_service.hpp"

#include <cstdint>
#include <iostream>
#include <string>

using namespace fetch;
using namespace fetch::subscribe;

int main(int /*argc*/, char const ** /*argv*/)
{
  // Networking needs this
  fetch::network::NetworkManager tm{"NetMgr", 5};

  uint16_t tcpPort = 8080;
  std::cout << "Starting subscribe server on tcp: " << tcpPort << std::endl;

  // Start our service
  SubscribeService serv(tm, tcpPort);
  tm.Start();

  for (std::size_t i = 0; i < 10; ++i)
  {
    std::cout << "Enter message to send to connected client(s)" << std::endl;
    std::string message;
    std::getline(std::cin, message);
    serv.SendMessage(message);
  }

  tm.Stop();
}
