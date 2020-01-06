#pragma once
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

#include "crypto/ecdsa.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/packet.hpp"

#include <chrono>
#include <deque>
#include <memory>
#include <thread>

uint16_t const BASE_MUDDLE_PORT = 1337;
uint16_t const BASE_HTTP_PORT   = 8100;

struct CertificateGenerator
{
  using Prover         = fetch::crypto::Prover;
  using ProverPtr      = std::shared_ptr<Prover>;
  using Certificate    = fetch::crypto::Prover;
  using CertificatePtr = std::shared_ptr<Certificate>;

  static ProverPtr New()
  {
    using Signer    = fetch::crypto::ECDSASigner;
    using SignerPtr = std::shared_ptr<Signer>;

    SignerPtr certificate = std::make_shared<Signer>();

    certificate->GenerateKeys();

    return certificate;
  }
};

struct Node
{
  using TrackerConfiguration = fetch::muddle::TrackerConfiguration;
  using ConstByteArray       = fetch::byte_array::ConstByteArray;
  using ByteArray            = fetch::byte_array::ByteArray;
  using Address              = typename fetch::muddle::Packet::Address;

  using NetworkManager    = fetch::network::NetworkManager;
  using NetworkManagerPtr = std::shared_ptr<NetworkManager>;
  using Uri               = fetch::network::Uri;
  using Payload           = fetch::muddle::Packet::Payload;
  using Certificate       = fetch::crypto::Prover;
  using CertificatePtr    = std::shared_ptr<Certificate>;
  using MuddlePtr         = fetch::muddle::MuddlePtr;

  using Milliseconds = std::chrono::milliseconds;

  explicit Node(uint16_t p)
    : network_manager{std::make_shared<NetworkManager>("NetMgr" + std::to_string(p), 1)}

  {
    port = p;
    network_manager->Start();

    char const *external         = std::getenv("MUDDLE_EXTERNAL");
    char const *external_address = (external == nullptr) ? "127.0.0.1" : external;
    certificate                  = CertificateGenerator::New();
    muddle  = fetch::muddle::CreateMuddle("TEST", certificate, *network_manager, external_address);
    address = muddle->GetAddress();

    muddle->Start({p});
    muddle->SetTrackerConfiguration(TrackerConfiguration::AllOn());
  }

  void Start(TrackerConfiguration configuration = {})
  {
    network_manager->Start();
    muddle->Start({port});
    muddle->SetTrackerConfiguration(configuration);
  }

  Node(Node const &other) = delete;
  Node const &operator=(Node const &other) = delete;

  void Stop()
  {
    muddle->Stop();
    network_manager->Stop();

    char const *external         = std::getenv("MUDDLE_EXTERNAL");
    char const *external_address = (external == nullptr) ? "127.0.0.1" : external;

    network_manager = std::make_shared<NetworkManager>("NetMgr" + std::to_string(port), 1);
    muddle = fetch::muddle::CreateMuddle("TEST", certificate, *network_manager, external_address);
  }

  NetworkManagerPtr network_manager;
  CertificatePtr    certificate;
  MuddlePtr         muddle;
  Address           address;
  uint16_t          port;
};

struct Network
{
  using TrackerConfiguration = fetch::muddle::TrackerConfiguration;
  using ConstByteArray       = fetch::byte_array::ConstByteArray;
  using ByteArray            = fetch::byte_array::ByteArray;
  using Address              = typename fetch::muddle::Packet::Address;

  using NetworkManager    = fetch::network::NetworkManager;
  using NetworkManagerPtr = std::shared_ptr<NetworkManager>;
  using Uri               = fetch::network::Uri;
  using Payload           = fetch::muddle::Packet::Payload;
  using Certificate       = fetch::crypto::Prover;
  using CertificatePtr    = std::unique_ptr<Certificate>;

  using Milliseconds = std::chrono::milliseconds;

  static std::unique_ptr<Network> New(uint64_t number_of_nodes, TrackerConfiguration config = {})
  {
    std::unique_ptr<Network> ret;
    ret.reset(new Network(number_of_nodes, config));
    return ret;
  }

  void Start(TrackerConfiguration configuration = {})
  {
    for (auto &node : nodes)
    {
      node->Start(configuration);
    }
  }

  void Stop()
  {
    for (auto &node : nodes)
    {
      node->Stop();
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  void Shutdown()
  {
    Stop();
    nodes.clear();
  }

  void AddNode(TrackerConfiguration config)
  {
    nodes.emplace_back(std::make_unique<Node>(static_cast<uint16_t>(BASE_MUDDLE_PORT + counter)));
    nodes.back()->muddle->SetTrackerConfiguration(config);
    nodes.back()->muddle->ConnectTo(
        fetch::network::Uri("tcp://127.0.0.1:" + std::to_string(BASE_MUDDLE_PORT + counter - 1)));

    ++counter;
  }

  void PopFrontNode()
  {
    auto &node = nodes.front();
    node->muddle->Stop();
    nodes.pop_front();
  }

  std::deque<std::unique_ptr<Node>> nodes;

private:
  explicit Network(uint64_t number_of_nodes, TrackerConfiguration config = {})
  {
    /// Creating the nodes
    for (uint64_t i = 0; i < number_of_nodes; ++i)
    {
      nodes.emplace_back(std::make_unique<Node>(static_cast<uint16_t>(BASE_MUDDLE_PORT + counter)));
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

inline void LinearConnectivity(
    std::unique_ptr<Network> &             network,
    fetch::muddle::Muddle::Duration const &expire =
        std::chrono::duration_cast<fetch::muddle::Muddle::Duration>(std::chrono::hours(1024 * 24)))
{
  auto N = network->nodes.size();
  for (std::size_t i = 1; i < N; ++i)
  {
    auto &node = *network->nodes[i];
    node.muddle->ConnectTo(
        fetch::network::Uri("tcp://127.0.0.1:" + std::to_string(BASE_MUDDLE_PORT - 1 + i)), expire);
  }
}

inline void AllToAllConnectivity(
    std::unique_ptr<Network> &             network,
    fetch::muddle::Muddle::Duration const &expire =
        std::chrono::duration_cast<fetch::muddle::Muddle::Duration>(std::chrono::hours(1024 * 24)))
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

inline fetch::muddle::Address FakeAddress(uint64_t i)
{
  fetch::muddle::Address ret;
  fetch::crypto::SHA256  hasher;

  hasher.Update(reinterpret_cast<uint8_t *>(&i), sizeof(uint64_t));
  ret = hasher.Final();

  return ret;
}
