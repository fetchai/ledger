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


/* Below we illustrate the architecture of the advanced RPC example:
 *       ┌─────────────────┐      ┌─────────────────┐
 *       │    TCP client   │  ... │    TCP client   │
 *       └───────────────▲─┘      └───▲─────────────┘
 *                        ╲          ╱                          ┌───┐
 *                         ╲        ╱                           │ T │
 *  aea_functionality.hpp   ╲      ╱  node_functionality.hpp    │ C │
 *  ┌────────────────────┐┌──╳────╳────────────┐     ┌───┐      │ P │
 *  │  AEA operation     ││  Node to node      │     │ T │  (2) │   │
 *  │  implementation    ││  functionality     │     │ C │◀═════▶ c │
 *  └──────────┬─────────┘└──────────┬─────────┘     │ P │      │ l │
 *             │      ┌──────────────┼───────────────▶   │      │ . │
 *  aea_protocol.hpp  │              │   ┌───────────▶ s │      └──▲┘
 *  ┌──────────▼──────▼──┐┌──────────▼───▼─────┐     │ e │(1)┌───┐ │
 *  │    AEA Protocol    ││ Node to node proto.│     │ . ◀═══▶ T │ │
 *  └────────────────────┘└────────────────────┘     └───┘   │ C │ │
 *             │                     │    node_protocol.hpp  │ P │ │
 *             └────────┐ ┌──────────┘                       │   │ │
 *  ┌───────────────────▼─▼────────────────────┐             │ c │ │
 *  │             Fetch service                │             │ l │ └───┐
 *  └─────────────────────┬────────────────────┘             │ . │     │
 *                        │              service.hpp         └───┘     │
 *  ┌─────────────────────▼────────────────────┐               ▲       │
 *  │              Node main program           │               │       │
 *  └──────────────────────────────────────────┘       ┌───────┘       │
 *                    node.cpp                         │               │
 *                                             ┌───────▼────────┐┌─────▼─────┐
 *  (1) use AEA protocol                       │   AEA client   ││   Node    │
 *  (2) use NodeToNode protocol                └────────────────┘└───────────┘
 *                                                  aea.cpp
 */

#include "commands.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/commandline/vt100.hpp"
#include "network/service/client.hpp"
#include "network/service/function.hpp"
#include "vector_serialize.hpp"
#include <iostream>

using namespace fetch::service;
using namespace fetch::commandline;

int main(int argc, char const **argv)
{
  ParamsParser params;
  params.Parse(argc, argv);

  if (params.arg_size() <= 1)
  {
    std::cout << "usage: ./" << argv[0] << " [command] [args ...]" << std::endl;
    std::cout << std::endl << "Commands are: " << std::endl;
    std::cout << "  connect [host] [[port=8080]]" << std::endl;
    std::cout << "  info" << std::endl;
    std::cout << "  listen" << std::endl;
    std::cout << "  sendmsg [msg]" << std::endl;
    std::cout << "  messages" << std::endl;
    std::cout << std::endl << "Params are" << std::endl;
    std::cout << "  --port=[1337]" << std::endl;
    std::cout << "  --host=[localhost]" << std::endl;
    std::cout << std::endl;

    return -1;
  }

  std::string command = params.GetArg(1);
  std::cout << std::endl;
  std::cout << "Executing command: " << command << std::endl;

  // Connecting to server
  uint16_t    port = params.GetParam<uint16_t>("port", 1337);
  std::string host = params.GetParam("host", "localhost");

  std::cout << "Connecting to server " << host << " on " << port << std::endl;

  fetch::network::NetworkManager tm;
  fetch::network::TCPClient      connection(tm);
  connection.Connect(host, port);

  ServiceClient client(connection, tm);

  tm.Start();

  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // Using the command protocol
  if (command == "connect")
  {
    if (params.arg_size() <= 2)
    {
      std::cout << "usage: ./" << argv[0] << " connect [host] [[port=8080]]" << std::endl;
      return -1;
    }

    std::string h = params.GetArg(2);
    uint16_t    p = params.GetArg<uint16_t>(3, 8080);
    std::cout << "Sending 'connect' command with parameters " << h << " " << p << std::endl;

    auto prom = client.Call(FetchProtocols::AEA_PROTOCOL, AEACommands::CONNECT, h, p);
    prom.Wait();
  }

  // Using the getinfo protocol
  if (command == "info")
  {

    std::cout << "Sending 'info' command with no parameters " << std::endl;

    auto prom = client.Call(FetchProtocols::AEA_PROTOCOL, AEACommands::GET_INFO);
    std::cout << "Info about the node: " << std::endl;
    std::cout << prom.As<std::string>() << std::endl << std::endl;
  }

  if (command == "listen")
  {
    std::cout << "Listening to " << std::endl;
    auto handle1 = client.Subscribe(FetchProtocols::PEER_TO_PEER, PeerToPeerFeed::NEW_MESSAGE,
                                    new Function<void(std::string)>([](std::string const &msg) {
                                      std::cout << VT100::GetColor("blue", "default")
                                                << "Got message: " << msg
                                                << VT100::DefaultAttributes() << std::endl;
                                    }));

    client.Subscribe(FetchProtocols::PEER_TO_PEER, PeerToPeerFeed::NEW_MESSAGE,
                     new Function<void(std::string)>([](std::string const &msg) {
                       std::cout << VT100::GetColor("red", "default") << "Got message 2: " << msg
                                 << VT100::DefaultAttributes() << std::endl;
                     }));

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    client.Unsubscribe(handle1);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    // Ungracefully leaving
  }

  // Testing the send message
  if (command == "sendmsg")
  {
    std::string msg = params.GetArg(2);
    std::cout << "Peer-to-peer command 'sendmsg' command with " << msg << std::endl;

    auto prom = client.Call(FetchProtocols::PEER_TO_PEER, PeerToPeerCommands::SEND_MESSAGE, msg);

    prom.Wait();
  }

  // Testing the send message
  if (command == "messages")
  {
    std::cout << "Peer-to-peer command 'messages' command with no parameters " << std::endl;
    auto prom = client.Call(FetchProtocols::PEER_TO_PEER, PeerToPeerCommands::GET_MESSAGES);

    std::vector<std::string> msgs = prom.As<std::vector<std::string>>();
    for (auto &msg : msgs)
    {
      std::cout << "  - " << msg << std::endl;
    }
  }

  tm.Stop();

  return 0;
}
