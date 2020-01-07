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

#include "kademlia/table.hpp"
#include "muddle.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"

#include "network_helpers.hpp"
#include "router.hpp"

#include "gtest/gtest.h"

TEST(RoutingTests, PeerTestReboot)
{
  auto        config = fetch::muddle::TrackerConfiguration::AllOn();
  std::size_t N      = 10;
  {
    // Clearing tables
    for (uint64_t idx = 0; idx < N; ++idx)
    {
      fetch::muddle::KademliaTable table(FakeAddress(10), fetch::muddle::NetworkId("TEST"));
      table.SetCacheFile("peer_table" + std::to_string(idx) + ".cache.db", false);
      table.Dump();
    }
  }

  auto network = Network::New(N, config);
  {
    // Creating network
    uint64_t idx = 0;
    for (auto &n : network->nodes)
    {
      n->muddle->SetPeerTableFile("peer_table" + std::to_string(idx) + ".cache.db");
      ++idx;
    }

    LinearConnectivity(network, std::chrono::seconds(5));

    // Waiting for the network to come up
    uint64_t q = 0;
    for (auto &n : network->nodes)
    {
      // Waiting up to 120 seconds for the connections to come around
      while ((n->muddle->GetNumDirectlyConnectedPeers() < config.max_kademlia_connections) &&
             (q < 300))
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        ++q;
      }

      EXPECT_GE(n->muddle->GetNumDirectlyConnectedPeers(), config.max_kademlia_connections);
    }
    std::cout << "Total setup: " << static_cast<double>(q * 400) / 1000. << " seconds" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(1400));

    std::cout << "==============================================================" << std::endl;
    std::cout << "===========================STOPPING===========================" << std::endl;
    std::cout << "==============================================================" << std::endl;
    network->Stop();
  }

  std::cout << "==============================================================" << std::endl;
  std::cout << "==========================REBOOTING===========================" << std::endl;
  std::cout << "==============================================================" << std::endl;
  network->Start(config);

  {
    // Restarting
    uint64_t idx = 0;
    for (auto &n : network->nodes)
    {
      n->muddle->SetPeerTableFile("peer_table" + std::to_string(idx) + ".cache.db");
      ++idx;
    }

    // We expect the total number of connections (in and out) that any one node has to be
    // be at least the maximum number of kademlia connections
    std::unordered_set<fetch::muddle::Address> all_addresses1;
    std::unordered_set<fetch::muddle::Address> all_addresses2;
    uint64_t                                   q = 0;

    std::cout << "==============================================================" << std::endl;
    std::cout << "===========================TESTING============================" << std::endl;
    std::cout << "==============================================================" << std::endl;

    for (auto &n : network->nodes)
    {

      // Waiting up to 40 seconds for the connections to come around
      while ((n->muddle->GetNumDirectlyConnectedPeers() < config.max_kademlia_connections) &&
             (q < 100))
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        ++q;
      }

      for (auto const &adr : n->muddle->GetDirectlyConnectedPeers())
      {
        all_addresses1.emplace(adr);
      }
      all_addresses2.emplace(n->address);

      EXPECT_GE(n->muddle->GetNumDirectlyConnectedPeers(), config.max_kademlia_connections);
    }
    std::cout << "Total delay: " << static_cast<double>(q * 400) / 1000. << " seconds" << std::endl;

    EXPECT_EQ(all_addresses1.size(), N);
    EXPECT_EQ(all_addresses1, all_addresses2);

    network->Stop();
  }
  network->Shutdown();
}
