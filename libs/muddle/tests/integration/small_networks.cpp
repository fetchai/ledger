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

#include "muddle.hpp"
#include "router.hpp"

#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "network/management/network_manager.hpp"

#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace {

using NetworkManager    = fetch::network::NetworkManager;
using NetworkManagerPtr = std::unique_ptr<NetworkManager>;
using Uri               = fetch::network::Uri;
using Muddle            = fetch::muddle::Muddle;
using MuddlePtr         = std::shared_ptr<Muddle>;
using Payload           = fetch::muddle::Packet::Payload;
using Address           = Muddle::Address;
using MuddlePtr         = std::shared_ptr<Muddle>;
using Certificate       = fetch::crypto::Prover;
using CertificatePtr    = std::unique_ptr<Certificate>;

using fetch::muddle::NetworkId;
using std::chrono::milliseconds;
using std::this_thread::sleep_for;

CertificatePtr NewCertificate()
{
  auto ret = std::make_unique<fetch::crypto::ECDSASigner>();
  ret->GenerateKeys();
  return ret;
}

struct Node
{
  Node(uint16_t port)
    : network_manager{std::make_unique<NetworkManager>("NetMgr" + std::to_string(port), 1)}
  {
    network_manager->Start();
    muddle = std::make_shared<Muddle>(NetworkId{"Test"}, NewCertificate(), *network_manager);
    muddle->Start({port});
  }

  void Stop()
  {
    muddle->Stop();
    network_manager->Stop();
  }

  NetworkManagerPtr network_manager;
  MuddlePtr         muddle;
  Address           address;
};

struct Network
{
  static std::unique_ptr<Network> New(uint64_t number_of_nodes)
  {
    std::unique_ptr<Network> ret;
    ret.reset(new Network(number_of_nodes));
    return ret;
  }

  void Stop()
  {
    for (auto &node : nodes)
    {
      node.Stop();
    }

    nodes.clear();
  }

  std::vector<Node> nodes;

private:
  Network(uint64_t number_of_nodes)
  {
    /// Creating the nodes
    for (uint64_t i = 0; i < number_of_nodes; ++i)
    {
      nodes.emplace_back(static_cast<uint16_t>(8000 + i));
    }
  }
};

void MakeKademliaNetwork(std::unique_ptr<Network> &network)
{
  for (auto &node : network->nodes)
  {
    node.muddle->SetPeerSelectionMode(fetch::muddle::PeerSelectionMode::KADEMLIA);
  }
}

void LinearConnectivity(std::unique_ptr<Network> &network)
{
  auto N = network->nodes.size();
  for (std::size_t i = 0; i < N - 1; ++i)
  {
    auto &node = network->nodes[i];
    node.muddle->ConnectTo(fetch::network::Uri("tcp://127.0.0.1:" + std::to_string(8001 + i)));
  }
}
/*
TEST(SmallNetworks, TestConnectivityNormalMode)
{
  uint64_t N       = 10;
  auto     network = Network::New(N);

  LinearConnectivity(network);
  sleep_for(std::chrono::milliseconds(2000));

  // End nodes would have 1 connected peer
  auto &n0 = network->nodes[0];
  EXPECT_EQ(n0.muddle->GetNumDirectlyConnectedPeers(), 1);

  auto &nN = network->nodes[N - 1];
  EXPECT_EQ(nN.muddle->GetNumDirectlyConnectedPeers(), 1);

  // For the rest we expect exactly 2 peers
  for (std::size_t i = 1; i < N - 1; ++i)
  {
    auto &node = network->nodes[i];
    EXPECT_EQ(node.muddle->GetNumDirectlyConnectedPeers(), 2);
  }

  // Sending messages

  network->Stop();
}
*/
/*
TEST(SmallNetworks, TestConnectivityKademliaMode)
{
  uint64_t N       = 10;
  auto     network = Network::New(N);

  MakeKademliaNetwork(network);
  LinearConnectivity(network);

  sleep_for(std::chrono::milliseconds(20000));

  // With 10 nodes and Kademlia peer selection
  // we would expect the muddle to automatically increase
  // its connectivity. Hence, in this we expect connectivity
  // to be larger than what is expected from manual connectivity
  // maintainance.
  auto &n0 = network->nodes[0];
  EXPECT_GT(n0.muddle->GetNumDirectlyConnectedPeers(), 1);

  auto &nN = network->nodes[N - 1];
  EXPECT_GT(nN.muddle->GetNumDirectlyConnectedPeers(), 1);

  // For the rest we expect at least 2 peers
  for (std::size_t i = 1; i < N - 1; ++i)
  {
    auto &node = network->nodes[i];
    EXPECT_GT(node.muddle->GetNumDirectlyConnectedPeers(), 2);
  }

  // Sending messages

  network->Stop();
}
*/
}  // namespace

#include "core/byte_array/encoders.hpp"
#include "core/random/lfg.hpp"
#include "crypto/sha256.hpp"
#include "muddle/kademlia/table.hpp"

using namespace fetch;
using ConstByteArray = byte_array::ConstByteArray;
using ByteArray      = byte_array::ByteArray;
using RawAddress     = typename muddle::Packet::RawAddress;
using KademliaTable  = muddle::KademliaTable;
using namespace fetch::muddle;

RawAddress FakeAddress(uint64_t i)
{
  RawAddress     ret;
  crypto::SHA256 hasher;

  hasher.Update(reinterpret_cast<uint8_t *>(&i), sizeof(uint64_t));
  hasher.Final(ret.data());

  return ret;
}

KademliaAddress FakeKademliaAddress(std::initializer_list<uint8_t> vals)
{
  KademliaAddress ret;
  uint64_t        i = vals.size();

  for (auto &v : vals)
  {
    --i;
    ret.words[i] = v;
    if (i == 0)
    {
      break;
    }
  }
  return ret;
}

ConstByteArray ReadibleAddress(RawAddress address)
{
  ByteArray ret;
  ret.Resize(address.size());
  std::memcpy(ret.pointer(), address.data(), address.size());
  return byte_array::ToBase64(ret);
}

ConstByteArray ReadibleKadAddress(KademliaAddress const &address)
{
  ByteArray ret;

  ret.Resize(address.size());
  std::memcpy(ret.pointer(), address.words, address.size());

  return byte_array::ToHex(ret);
}

ConstByteArray ReadibleDistance(KademliaDistance const &dist)
{
  ByteArray ret;

  ret.Resize(dist.size());
  std::memcpy(ret.pointer(), dist.data(), dist.size());

  return byte_array::ToHex(ret);
}

TEST(SmallNetworks, BasicAddressTests)
{
  auto zero_address = FakeKademliaAddress({});
  EXPECT_EQ(GetBucket(GetKademliaDistance(zero_address, zero_address)), 0);

  {
    auto address = FakeKademliaAddress({255});
    EXPECT_EQ(GetBucket(GetKademliaDistance(zero_address, address)), 8);
  }

  {
    auto address = FakeKademliaAddress({255, 255});
    EXPECT_EQ(GetBucket(GetKademliaDistance(zero_address, address)), 16);
  }

  {
    auto address = FakeKademliaAddress({static_cast<uint8_t>(1 << 7), 0});
    EXPECT_EQ(GetBucket(GetKademliaDistance(zero_address, address)), 16);
  }

  {
    auto address = FakeKademliaAddress({static_cast<uint8_t>(1 << 4), 0, 0});
    EXPECT_EQ(GetBucket(GetKademliaDistance(zero_address, address)), 21);
  }
}

TEST(SmallNetworks, KademliaPrimitives)
{
  fetch::random::LaggedFibonacciGenerator<> lfg;

  auto raw_address1 = FakeAddress(1);
  auto raw_address2 = FakeAddress(2);

  auto kam_address1 = KademliaAddress::Create(raw_address1);
  auto kam_address2 = KademliaAddress::Create(raw_address2);
  auto dist         = GetKademliaDistance(kam_address1, kam_address2);

  EXPECT_EQ(GetBucket(GetKademliaDistance(kam_address1, kam_address1)), 0);

  constexpr uint64_t const N = 1000;

  KademliaTable table(raw_address1);

  std::vector<PeerInfo> all_peers;
  all_peers.resize(N);
  uint64_t i = 1;

  for (auto &p : all_peers)
  {
    p.address          = FakeAddress(i);
    p.kademlia_address = KademliaAddress::Create(p.address);
    ++i;
  }

  // Simulating a 1 peers simulating a 100.000 interactions
  for (std::size_t i = 0; i < 10 * N; ++i)
  {
    table.ReportLiveliness(all_peers[lfg() % N].address);
  }

  // Testing the table
  std::vector<PeerInfo> ref_peers = all_peers;
  for (auto &p1 : ref_peers)
  {
    // Sorting all peers according to the distance to p1
    for (auto &p2 : all_peers)
    {
      p2.distance = GetKademliaDistance(p1.kademlia_address, p2.kademlia_address);
    }
    std::sort(all_peers.begin(), all_peers.end());

    // Testing that the set
    auto found_peers = table.FindPeer(p1.address);
    /*
    for (std::size_t i = 0; i < 2; ++i)
    {
      if (found_peers[i].address != all_peers[i].address)
      {
        std::cout << "NO MATCH" << std::endl;
      }
      else
      {
        std::cout << "OK" << std::endl;
      }
    }
    */
  }

  std::cout << "Getting nearest nodes for peer 10: " << table.FindPeer(FakeAddress(10)).size()
            << std::endl;
}
