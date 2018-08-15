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

#include <iostream>

#include "core/commandline/parameter_parser.hpp"
#include "core/commandline/vt100.hpp"

#include "./quick_start_service.hpp"
#include "network/management/network_manager.hpp"

using namespace fetch;
using namespace fetch::commandline;
using namespace fetch::quick_start;

int main(int argc, char const **argv)
{
  // Networking needs this
  fetch::network::NetworkManager tm(5);

  ParamsParser params;
  params.Parse(argc, argv);

  // get our port and the port of the remote we want to connect to
  if (params.arg_size() <= 1)
  {
    std::cout << "usage: ./" << argv[0] << " [params ...]" << std::endl;
    std::cout << std::endl << "Params are" << std::endl;
    std::cout << "  --port=[8000]" << std::endl;
    std::cout << "  --remotePort=[8001]" << std::endl;
    std::cout << std::endl;

    return -1;
  }

  uint16_t tcpPort    = uint16_t(std::stoi(params.GetArg(1)));
  uint16_t remotePort = uint16_t(std::stoi(params.GetArg(2)));
  std::cout << "Starting server on tcp: " << tcpPort << " connecting to: " << remotePort
            << std::endl;

  // Start our service
  QuickStartService serv(tm, tcpPort);
  tm.Start();

  for (std::size_t i = 0; i < 10; ++i)
  {
    std::cout << "Enter message to send to connected node(s)" << std::endl;
    std::string message;
    std::getline(std::cin, message);
    serv.sendMessage(message, remotePort);
  }

  tm.Stop();
}
