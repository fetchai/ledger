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

#include "muddle.hpp"
#include "router.hpp"

#include "core/byte_array/encoders.hpp"
#include "core/random/lfg.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "crypto/sha256.hpp"
#include "kademlia/address_priority.hpp"
#include "kademlia/peer_tracker.hpp"
#include "kademlia/table.hpp"
#include "network/management/network_manager.hpp"

#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

using namespace fetch;
using ConstByteArray = byte_array::ConstByteArray;
using ByteArray      = byte_array::ByteArray;
using Address        = typename muddle::Packet::Address;
using KademliaTable  = muddle::KademliaTable;
using namespace fetch::muddle;

namespace {

using NetworkManager    = fetch::network::NetworkManager;
using NetworkManagerPtr = std::unique_ptr<NetworkManager>;
using Uri               = fetch::network::Uri;
using Muddle            = fetch::muddle::Muddle;
using MuddlePtr         = std::shared_ptr<Muddle>;
using Payload           = fetch::muddle::Packet::Payload;
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
  Node(uint16_t p, TrackerConfiguration const &configuration)
    : network_manager{std::make_unique<NetworkManager>("NetMgr" + std::to_string(p), 1)}
    , port{p}
  {
    network_manager->Start();
    muddle = std::make_shared<Muddle>(NetworkId{"Test"}, NewCertificate(), *network_manager);
    muddle->Start({port});
    muddle->SetTrackerConfiguration(configuration);
  }

  void Stop()
  {
    muddle->Stop();
    network_manager->Stop();
  }

  NetworkManagerPtr network_manager;
  MuddlePtr         muddle;
  Address           address;
  uint16_t          port;
};

struct Network
{
  static std::unique_ptr<Network> New(uint64_t                    number_of_nodes,
                                      TrackerConfiguration const &configuration = {},
                                      uint16_t                    offset        = 8000)
  {
    std::unique_ptr<Network> ret;
    ret.reset(new Network(number_of_nodes, configuration, offset));
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
  uint16_t          port_offset;

private:
  Network(uint64_t number_of_nodes, TrackerConfiguration const &configuration, uint16_t offset)
    : port_offset{offset}

  {
    /// Creating the nodes
    for (uint64_t i = 0; i < number_of_nodes; ++i)
    {
      nodes.emplace_back(static_cast<uint16_t>(offset + i), configuration);
    }
  }
};

void MakeKademliaNetwork(std::unique_ptr<Network> &network)
{
  for (auto &node : network->nodes)
  {
    node.muddle->SetTrackerConfiguration(TrackerConfiguration::AllOn());
  }
}

void LinearConnectivity(std::unique_ptr<Network> &network)
{
  auto N = network->nodes.size();
  for (std::size_t i = 0; i < N - 1; ++i)
  {
    auto &node = network->nodes[i];
    node.muddle->ConnectTo(fetch::network::Uri("tcp://127.0.0.1:" + std::to_string(8001 + i)),
                           fetch::muddle::Muddle::NeverExpire());
  }
}

void ConnectNetworks(std::unique_ptr<Network> &n1, std::unique_ptr<Network> &n2)
{
  for (auto &node1 : n1->nodes)
  {
    for (auto &node2 : n2->nodes)
    {
      node1.muddle->ConnectTo(fetch::network::Uri("tcp://127.0.0.1:" + std::to_string(node2.port)),
                              fetch::muddle::Muddle::NeverExpire());
    }
  }
}

Address FakeAddress(uint64_t i)
{
  Address        ret;
  crypto::SHA256 hasher;

  hasher.Update(reinterpret_cast<uint8_t *>(&i), sizeof(uint64_t));
  ret = hasher.Final();

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

/*
ConstByteArray ReadibleAddress(Address const &address)
{
  ByteArray ret;
  ret.Resize(address.size());
  std::memcpy(ret.pointer(), address.pointer(), address.size());
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
*/

// TODO(tfr): move to unit test
TEST(SmallNetworks, DISABLED_OrganisingAddressPriority)
{
  AddressPriority optimal_connection;
  optimal_connection.address          = FakeAddress(0);
  optimal_connection.persistent       = true;
  optimal_connection.bucket           = 1;  // Good location
  optimal_connection.connection_value = 1;  // Good behavious
  optimal_connection.connected_since =
      AddressPriority::Clock::now() - std::chrono::seconds(4 * 3600);
  optimal_connection.UpdatePriority();

  // Good location, good behaviour and long term service should give close to
  // top rating. Naturally, we expect that persistent connection
  // is preferable
  EXPECT_LT(0.96, optimal_connection.priority);

  AddressPriority mediocre_loc;
  mediocre_loc.address          = FakeAddress(0);
  mediocre_loc.persistent       = true;
  mediocre_loc.bucket           = 1;  // Good location
  mediocre_loc.connection_value = 0;  // Not good, nor bad.
  mediocre_loc.connected_since  = AddressPriority::Clock::now() - std::chrono::seconds(4 * 3600);
  mediocre_loc.UpdatePriority();

  // Good location, but no evidence of good behaviour should put
  // a node somewhere around the middle
  EXPECT_LT(0.4, mediocre_loc.priority);
  EXPECT_LT(mediocre_loc.priority, 0.6);

  AddressPriority mediocre_beh;
  mediocre_beh.address          = FakeAddress(0);
  mediocre_beh.persistent       = true;
  mediocre_beh.bucket           = static_cast<uint64_t>(-1);  // Bad location
  mediocre_beh.connection_value = 1;                          // Good behaviour
  mediocre_beh.connected_since  = AddressPriority::Clock::now() - std::chrono::seconds(4 * 3600);
  mediocre_beh.UpdatePriority();

  // We value good behaviour slightly worse than good location
  EXPECT_LT(0.3, mediocre_beh.priority);
  EXPECT_LT(mediocre_beh.priority, 0.5);

  // We don't expect good behaviour alone to account
  // for persistent connection.

  AddressPriority optimal_gone_bad;
  optimal_gone_bad.address          = FakeAddress(0);
  optimal_gone_bad.persistent       = true;
  optimal_gone_bad.bucket           = 1;   // Good location
  optimal_gone_bad.connection_value = -1;  // Very bad behaviour
  optimal_gone_bad.connected_since = AddressPriority::Clock::now() - std::chrono::seconds(4 * 3600);
  optimal_gone_bad.UpdatePriority();

  // Getting lowest ranking should immediately drag you to the bottom
  // 1% of the nodes.
  EXPECT_LT(optimal_gone_bad.priority, 0.01);

  AddressPriority long_term_disconnect = optimal_connection;
  long_term_disconnect.ScheduleDisconnect();
  long_term_disconnect.UpdatePriority();
  EXPECT_LT(long_term_disconnect.priority, 0.05);

  AddressPriority poor_permanent;
  poor_permanent.address          = FakeAddress(0);
  poor_permanent.persistent       = true;
  poor_permanent.bucket           = static_cast<uint64_t>(-1);
  poor_permanent.connection_value = 0;
  poor_permanent.connected_since  = AddressPriority::Clock::now() - std::chrono::seconds(30);
  poor_permanent.UpdatePriority();
  EXPECT_LT(0.05, poor_permanent.priority);
  EXPECT_LT(poor_permanent.priority, 0.10);

  AddressPriority good_temporary;
  good_temporary.address          = FakeAddress(0);
  good_temporary.persistent       = false;
  good_temporary.connection_value = 0;
  good_temporary.desired_expiry   = AddressPriority::Clock::now() + std::chrono::seconds(30);
  good_temporary.connected_since  = AddressPriority::Clock::now() - std::chrono::seconds(30);
  good_temporary.bucket           = static_cast<uint64_t>(-1);
  good_temporary.UpdatePriority();

  AddressPriority good_temporary_close_to_expiry;
  good_temporary_close_to_expiry.address          = FakeAddress(0);
  good_temporary_close_to_expiry.persistent       = false;
  good_temporary_close_to_expiry.connection_value = 0;
  good_temporary_close_to_expiry.desired_expiry =
      AddressPriority::Clock::now() + std::chrono::seconds(1);
  good_temporary_close_to_expiry.connected_since =
      AddressPriority::Clock::now() - std::chrono::seconds(59);
  good_temporary_close_to_expiry.bucket = static_cast<uint64_t>(-1);
  good_temporary_close_to_expiry.UpdatePriority();

  // We expect a connection close to expiry to have lower priority
  // than one with high priority.
  EXPECT_LT(good_temporary_close_to_expiry, good_temporary);

  AddressPriority good_temporary_should_upgrade;
  good_temporary_should_upgrade.address          = FakeAddress(0);
  good_temporary_should_upgrade.persistent       = false;
  good_temporary_should_upgrade.connection_value = 0;
  good_temporary_should_upgrade.desired_expiry   = AddressPriority::Clock::now();
  -std::chrono::seconds(30);
  good_temporary_should_upgrade.connected_since =
      AddressPriority::Clock::now() - std::chrono::seconds(60);
  good_temporary_should_upgrade.bucket = static_cast<uint64_t>(1);
  good_temporary_should_upgrade.UpdatePriority();

  // We expect anew temporary connection to exceed another one
  // if the
  EXPECT_LT(good_temporary_should_upgrade.priority, good_temporary.priority);

  EXPECT_LT(0.10, good_temporary.priority);
  EXPECT_LT(mediocre_loc, optimal_connection);
  EXPECT_LT(optimal_gone_bad, mediocre_loc);
  EXPECT_LT(optimal_gone_bad, long_term_disconnect);
  EXPECT_LT(optimal_gone_bad, good_temporary);
  EXPECT_LT(poor_permanent, good_temporary);
}

TEST(SmallNetworks, DISABLED_NetworkRegistrationThreeLayers)
{
  TrackerConfiguration configuration = TrackerConfiguration::AllOff();
  configuration.register_connections = true;
  configuration.pull_peers           = true;

  {
    uint64_t N        = 5;
    auto     network1 = Network::New(N, configuration);
    auto     network2 = Network::New(1, configuration, 8100);
    auto     network3 = Network::New(N, configuration, 8200);

    ConnectNetworks(network1, network2);
    ConnectNetworks(network2, network3);

    auto &tracker = network2->nodes[0].muddle->peer_tracker();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    auto total = network1->nodes.size() + network2->nodes.size() + network3->nodes.size();
    EXPECT_EQ(tracker.known_peer_count(), total);
  }
}

TEST(SmallNetworks, DISABLED_NetworkRegistrationFiveLayers)
{
  TrackerConfiguration configuration = TrackerConfiguration::AllOff();
  configuration.register_connections = true;
  configuration.pull_peers           = true;
  // Testing propagation in deeper networks
  {
    uint64_t N        = 5;
    auto     network1 = Network::New(N, configuration);
    auto     network2 = Network::New(1, configuration, 8100);
    auto     network3 = Network::New(N, configuration, 8200);
    auto     network4 = Network::New(1, configuration, 8300);
    auto     network5 = Network::New(N, configuration, 8400);

    ConnectNetworks(network1, network2);
    ConnectNetworks(network2, network3);
    ConnectNetworks(network3, network4);
    ConnectNetworks(network4, network5);

    auto &tracker1 = network2->nodes[0].muddle->peer_tracker();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    auto &tracker2 = network4->nodes[0].muddle->peer_tracker();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    auto total = network1->nodes.size() + network2->nodes.size() + network3->nodes.size() +
                 network4->nodes.size() + network5->nodes.size();

    // Note that the total number of nodes cannot exceed 20.
    ASSERT_LT(total, 20);

    EXPECT_EQ(tracker1.known_peer_count(), total);
    EXPECT_EQ(tracker2.known_peer_count(), total);
  }
}

// Testing that the effect is not there when the configuration is turned off.
TEST(SmallNetworks, DISABLED_NetworkRegistrationOff)
{
  TrackerConfiguration configuration = TrackerConfiguration::AllOff();
  configuration.register_connections = false;
  configuration.pull_peers           = false;

  {
    uint64_t N        = 5;
    auto     network1 = Network::New(N, configuration);
    auto     network2 = Network::New(1, configuration, 8100);
    auto     network3 = Network::New(N, configuration, 8200);

    ConnectNetworks(network1, network2);
    ConnectNetworks(network2, network3);

    auto &tracker = network2->nodes[0].muddle->peer_tracker();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    auto total = 0;
    EXPECT_EQ(tracker.known_peer_count(), total);
  }
}

TEST(SmallNetworks, DISABLED_NetworkRegistrationRegistrationNoPull)
{
  TrackerConfiguration configuration = TrackerConfiguration::AllOff();
  // Testing that register connection has the effect expected
  configuration.register_connections = true;
  configuration.pull_peers           = false;
  {
    uint64_t N        = 5;
    auto     network1 = Network::New(N, configuration);
    auto     network2 = Network::New(1, configuration, 8100);
    auto     network3 = Network::New(N, configuration, 8200);
    auto     network4 = Network::New(1, configuration, 8300);
    auto     network5 = Network::New(N, configuration, 8400);

    ConnectNetworks(network1, network2);
    ConnectNetworks(network2, network3);
    ConnectNetworks(network3, network4);
    ConnectNetworks(network4, network5);

    auto &tracker1 = network2->nodes[0].muddle->peer_tracker();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    auto &tracker2 = network4->nodes[0].muddle->peer_tracker();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    auto total1 = network1->nodes.size() + network3->nodes.size();
    auto total2 = network3->nodes.size() + network5->nodes.size();

    EXPECT_EQ(tracker1.known_peer_count(), total1);
    EXPECT_EQ(tracker2.known_peer_count(), total2);
  }
}

TEST(SmallNetworks, DISABLED_TestConnectivityKademliaMode)
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

}  // namespace

TEST(SmallNetworks, DISABLED_BasicAddressTests)
{
  auto zero_address = FakeKademliaAddress({});
  EXPECT_EQ(Bucket::IdByLogarithm(GetKademliaDistance(zero_address, zero_address)), 0);

  {
    auto address = FakeKademliaAddress({255});
    EXPECT_EQ(Bucket::IdByLogarithm(GetKademliaDistance(zero_address, address)), 8);
  }

  {
    auto address = FakeKademliaAddress({255, 255});
    EXPECT_EQ(Bucket::IdByLogarithm(GetKademliaDistance(zero_address, address)), 16);
  }

  {
    auto address = FakeKademliaAddress({static_cast<uint8_t>(1 << 7), 0});
    EXPECT_EQ(Bucket::IdByLogarithm(GetKademliaDistance(zero_address, address)), 16);
  }

  {
    auto address = FakeKademliaAddress({static_cast<uint8_t>(1 << 4), 0, 0});
    EXPECT_EQ(Bucket::IdByLogarithm(GetKademliaDistance(zero_address, address)), 21);
  }
}

TEST(SmallNetworks, DISABLED_KademliaPrimitives)
{
  fetch::random::LaggedFibonacciGenerator<> lfg;

  auto raw_address1 = FakeAddress(1);
  auto raw_address2 = FakeAddress(2);

  auto kam_address1 = KademliaAddress::Create(raw_address1);

  EXPECT_EQ(Bucket::IdByLogarithm(GetKademliaDistance(kam_address1, kam_address1)), 0);

  constexpr uint64_t const N = 1000;

  KademliaTable table(raw_address1, fetch::muddle::NetworkId("TEST"));

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
  for (std::size_t j = 0; j < 10 * N; ++j)
  {
    table.ReportLiveliness(all_peers[lfg() % N].address, all_peers[lfg() % N].address);
  }

  // Testing the table
  // TODO(tfr): Turn into proper test
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
  }

  std::cout << "Getting nearest nodes for peer 10: " << table.FindPeer(FakeAddress(10)).size()
            << std::endl;
}
