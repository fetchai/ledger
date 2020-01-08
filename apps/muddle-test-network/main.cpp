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

#include "muddle/muddle_interface.hpp"
#include "muddle/packet.hpp"

#include "constellation/muddle_status_http_module.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/random/lfg.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "crypto/sha256.hpp"
#include "http/middleware/allow_origin.hpp"
#include "http/middleware/telemetry.hpp"
#include "http/module.hpp"
#include "http/server.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "network/management/network_manager.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

using namespace fetch;
using ConstByteArray = byte_array::ConstByteArray;
using ByteArray      = byte_array::ByteArray;
using Address        = typename muddle::Packet::Address;
using namespace fetch::muddle;

using NetworkManager     = fetch::network::NetworkManager;
using NetworkManagerPtr  = std::shared_ptr<NetworkManager>;
using Uri                = fetch::network::Uri;
using Payload            = fetch::muddle::Packet::Payload;
using Certificate        = fetch::crypto::Prover;
using CertificatePtr     = std::unique_ptr<Certificate>;
using MuddleStatusModule = fetch::constellation::MuddleStatusModule;

using std::chrono::milliseconds;
using std::this_thread::sleep_for;
using namespace fetch::http;

CertificatePtr NewCertificate()
{
  auto ret = std::make_unique<fetch::crypto::ECDSASigner>();
  ret->GenerateKeys();
  return ret;
}

struct Node
{
  using HttpModules = std::vector<std::shared_ptr<HTTPModule>>;

  Node(uint16_t port, uint16_t http_port)
    : network_manager{std::make_shared<NetworkManager>("NetMgr" + std::to_string(port), 1)}
    , http_network_manager{std::make_shared<NetworkManager>("HttpMgr" + std::to_string(http_port),
                                                            1)}
    , http{*http_network_manager}
    , http_modules{std::make_shared<MuddleStatusModule>()}

  {
    network_manager->Start();

    char const *external         = std::getenv("MUDDLE_EXTERNAL");
    char const *external_address = (external == nullptr) ? "127.0.0.1" : external;
    muddle  = muddle::CreateMuddle("TEST", NewCertificate(), *network_manager, external_address);
    address = muddle->GetAddress();

    uri = fetch::network::Uri("tcp://127.0.0.1:" + std::to_string(port));

    muddle->Start({port});
    muddle->SetTrackerConfiguration(TrackerConfiguration::AllOn());

    http_network_manager->Start();
    http.AddMiddleware(http::middleware::AllowOrigin("*"));
    http.AddMiddleware(http::middleware::Telemetry());

    for (auto const &module : http_modules)
    {
      http.AddModule(*module);
    }
    http.Start(http_port);
  }

  Node(Node const &other) = delete;
  Node const &operator=(Node const &other) = delete;

  void Stop()
  {
    http.Stop();
    http_network_manager->Stop();
    muddle->Stop();
    network_manager->Stop();
  }

  NetworkManagerPtr network_manager;
  MuddlePtr         muddle;
  Address           address;
  Uri               uri;

  NetworkManagerPtr http_network_manager;  ///< A separate net. coordinator for the http service(s)
  HTTPServer        http;                  ///< The HTTP server
  HttpModules       http_modules;          ///< The set of modules currently configured
};

uint16_t const BASE_MUDDLE_PORT = 1337;
uint16_t const BASE_HTTP_PORT   = 8100;

struct Network
{
  static std::unique_ptr<Network> New(uint64_t number_of_nodes, TrackerConfiguration config = {})
  {
    std::unique_ptr<Network> ret;
    ret.reset(new Network(number_of_nodes, config));
    return ret;
  }

  void Stop()
  {
    for (auto &node : nodes)
    {
      node->Stop();
    }

    nodes.clear();
  }

  void AddNode(TrackerConfiguration config)
  {
    auto uri =
        fetch::network::Uri("tcp://127.0.0.1:" + std::to_string(BASE_MUDDLE_PORT + counter - 1));
    nodes.emplace_back(std::make_unique<Node>(static_cast<uint16_t>(BASE_MUDDLE_PORT + counter),
                                              static_cast<uint16_t>(BASE_HTTP_PORT + counter)));
    nodes.back()->muddle->SetTrackerConfiguration(config);
    nodes.back()->muddle->ConnectTo(uri);
    ++counter;
  }

  std::vector<std::unique_ptr<Node>> nodes;

private:
  explicit Network(uint64_t number_of_nodes, TrackerConfiguration config = {})
  {
    /// Creating the nodes
    for (uint64_t i = 0; i < number_of_nodes; ++i)
    {
      nodes.emplace_back(std::make_unique<Node>(static_cast<uint16_t>(BASE_MUDDLE_PORT + counter),
                                                static_cast<uint16_t>(BASE_HTTP_PORT + counter)));
      nodes.back()->muddle->SetTrackerConfiguration(config);
      ++counter;
    }
  }

  uint64_t counter{0};
};

inline void MakeKademliaNetwork(std::unique_ptr<Network> &network)
{
  for (auto &node : network->nodes)
  {
    node->muddle->SetTrackerConfiguration(fetch::muddle::TrackerConfiguration::AllOn());
  }
}

inline void LinearConnectivity(std::unique_ptr<Network> &               network,
                               muddle::MuddleInterface::Duration const &expire =
                                   std::chrono::duration_cast<muddle::MuddleInterface::Duration>(
                                       std::chrono::hours(1024 * 24)))
{
  auto N = network->nodes.size();
  for (std::size_t i = 1; i < N; ++i)
  {
    auto &node = *network->nodes[i];
    node.muddle->ConnectTo(
        fetch::network::Uri("tcp://127.0.0.1:" + std::to_string(BASE_MUDDLE_PORT - 1 + i)), expire);
  }
}

inline void AllToAllConnectivity(std::unique_ptr<Network> &               network,
                                 muddle::MuddleInterface::Duration const &expire =
                                     std::chrono::duration_cast<muddle::MuddleInterface::Duration>(
                                         std::chrono::hours(1024 * 24)))
{
  auto N = network->nodes.size();
  for (std::size_t i = 0; i < N; ++i)
  {
    auto &node = *network->nodes[i];
    for (std::size_t j = 0; j < N; ++j)
    {

      node.muddle->ConnectTo(
          fetch::network::Uri("tcp://127.0.0.1:" + std::to_string(BASE_MUDDLE_PORT + j)), expire);
    }
  }
}

int mainXX()
{
  auto config                      = fetch::muddle::TrackerConfiguration::AllOn();
  config.max_kademlia_connections  = 2;
  config.max_longrange_connections = 1;

  uint64_t N       = 10;
  auto     network = Network::New(N, config);

  //  MakeKademliaNetwork(network);
  LinearConnectivity(network, std::chrono::seconds(5));

  uint64_t step = N / 4;
  for (uint64_t i = 0; i < N; i += step)
  {
    uint64_t next    = (i + step) % N;
    auto     address = network->nodes[next]->address;

    network->nodes[i]->muddle->ConnectTo(address);
  }

  std::string input;
  while (true)
  {
    for (uint64_t i = 0; i < N; ++i)
    {
      auto &n1 = network->nodes[i];
      for (uint64_t j = 0; j < N; ++j)
      {
        if (i == j)
        {
          continue;
        }
        auto &n2 = network->nodes[j];
        n1->muddle->ConnectTo(n2->address, n2->uri);
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  std::getline(std::cin, input);

  network->Stop();
  return 0;
}

int main()
{
  auto config                      = fetch::muddle::TrackerConfiguration::AllOn();
  config.max_kademlia_connections  = 2;
  config.max_longrange_connections = 1;

  uint64_t N       = 40;
  auto     network = Network::New(N, config);

  //  MakeKademliaNetwork(network);
  LinearConnectivity(network, std::chrono::seconds(5));

  uint64_t step = N / 4;
  for (uint64_t i = 0; i < N; i += step)
  {
    uint64_t next    = (i + step) % N;
    auto     address = network->nodes[next]->address;

    network->nodes[i]->muddle->ConnectTo(address, std::chrono::seconds(90));
  }

  std::string input;
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::seconds(400));
    network->AddNode({});
  }
  std::getline(std::cin, input);

  network->Stop();
  return 0;
}
