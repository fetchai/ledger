//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/serializers/base_types.hpp"
#include "network/tcp/tcp_client.hpp"

#include <chrono>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

using namespace fetch::network;

class Client : public TCPClient
{
public:
  Client(std::string const &host, std::string const &port, NetworkManager tmanager)
    : TCPClient(tmanager)  // NOLINT
  {
    Connect(host, port);
    this->OnMessage([](MessageBuffer const &value) { std::cout << value << std::endl; });
    this->OnConnectionFailed([]() { std::cerr << "Connection failed" << std::endl; });
  }
};

int main(int argc, char *argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: client <host> <port>\n";
      return 1;
    }
    NetworkManager tmanager{"NetMgr", 1};
    tmanager.Start();

    // Attempt to break the connection
    for (std::size_t i = 0; i < 1000; ++i)
    {
      Client client(argv[1], argv[2], tmanager);

      while (!client.is_alive())
      {
        std::cout << "Waiting for client to connect" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }

      fetch::byte_array::ByteArray msg0("Testing rapid string pushing");
      client.Send(msg0.Copy());
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    Client                       client(argv[1], argv[2], tmanager);
    fetch::byte_array::ByteArray msg;
    msg.Resize(512);

    while (std::cin.getline(msg.char_pointer(), 512))
    {
      msg.Resize(std::strlen(msg.char_pointer()));
      client.Send(msg.Copy());
      msg.Resize(512);
    }

    tmanager.Stop();
  }
  catch (std::exception const &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
