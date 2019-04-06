#pragma once
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

#include "network/service/service_client.hpp"
#include "protocols/fetch_protocols.hpp"
#include "protocols/quick_start/protocol.hpp"  // defines our quick start protocol

namespace fetch {
namespace quick_start {

using clientType = service::ServiceClient;

// Custom class we want to pass using the RPC interface
class DataClass
{
public:
  std::vector<int> data_;
};

class Node
{
public:
  Node(fetch::network::NetworkManager tm)
    : tm_{tm}
  {}
  ~Node()
  {}

  void sendMessage(std::string const &msg, uint16_t port)
  {
    std::cout << "\nNode sending: \"" << msg << "\" to: " << port << std::endl;

    fetch::network::TCPClient connection(tm_);
    connection.Connect("localhost", port);

    clientType client(connection, tm_);

    for (std::size_t i = 0;; ++i)
    {
      if (client.is_alive())
      {
        break;
      }
      std::cout << "Waiting for client to connect..." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      if (i == 10)
      {
        std::cout << "Failed to connect to client" << std::endl;
        return;
      }
    }

    // Ping the client
    client.Call(protocols::QuickStartProtocols::QUICK_START, protocols::QuickStart::PING);

    // Call the SEND_MESSAGE function using the QUICK_START protocol
    // sending msg, and getting result (calls receiveMessage)
    int result = client
                     .Call(protocols::QuickStartProtocols::QUICK_START,
                           protocols::QuickStart::SEND_MESSAGE, msg)
                     ->As<int>();

    std::cout << "Remote responded: " << result << std::endl;

    // Send data using our custom class
    DataClass d;
    d.data_   = {1, 2, 3};
    auto prom = client.Call(protocols::QuickStartProtocols::QUICK_START,
                            protocols::QuickStart::SEND_DATA, d);

    FETCH_LOG_PROMISE();
    prom->Wait();
  }

  ////////////////////////////////////////////
  // Functions exposed via RPC

  // This is called when nodes are calling SEND_MESSAGE on us
  int receiveMessage(std::string const &msg)
  {
    std::cout << "Node received: " << msg << std::endl;
    static int count{0};
    return count++;
  }

  void receiveData(DataClass const &data)
  {
    std::cout << "Received data:" << std::endl;
    for (auto &i : data.data_)
    {
      std::cout << i << std::endl;
    }
  }

  void ping()
  {
    std::cout << "We have been pinged!" << std::endl;
  }

private:
  fetch::network::NetworkManager tm_;
};

// All classes and data types must have an associated Serialize and Deserialize
// function So we define one for our class (vector already has a ser/deser)
template <typename T>
inline void Serialize(T &serializer, DataClass const &data)
{
  serializer << data.data_;
}

template <typename T>
inline void Deserialize(T &serializer, DataClass &data)
{
  serializer >> data.data_;
}

}  // namespace quick_start
}  // namespace fetch
