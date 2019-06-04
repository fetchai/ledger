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

#include "network/tcp/tcp_server.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <thread>

using namespace fetch::network;

class Server : public TCPServer
{
public:
  Server(uint16_t p, NetworkManager tmanager)
    : TCPServer(p, tmanager)
  {}

  void PushRequest(connection_handle_type /*client*/, message_type const &msg) override
  {
    std::cout << "Message: " << msg << std::endl;
  }
};

int main(int argc, char *argv[])
{
  try
  {

    if (argc != 2)
    {
      std::cerr << "Usage: rpc_server <port>\n";
      return 1;
    }

    NetworkManager tmanager{"NetMgr", 1};

    Server s(uint16_t(std::atoi(argv[1])), tmanager);
    tmanager.Start();
    s.Start();

    std::cout << "Press Ctrl+C to quit" << std::endl;

    while (true)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    s.Stop();
    tmanager.Stop();
  }
  catch (std::exception const &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
