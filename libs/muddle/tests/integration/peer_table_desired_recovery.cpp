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

TEST(RoutingTests, DISABLED_DesiredTableAfterReboot)
{
  std::vector<fetch::muddle::Address> desired_peers;

  auto        config = fetch::muddle::TrackerConfiguration::AllOn();
  std::size_t N      = 10;
  std::size_t K      = 4;
  std::size_t M      = N / K + 1;

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
      if ((idx % K) == 0)
      {
        if (!desired_peers.empty())
        {
          n->muddle->ConnectTo(desired_peers.back());
        }

        desired_peers.push_back(n->address);
      }

      n->muddle->SetPeerTableFile("peer_table" + std::to_string(idx) + ".cache.db");
      ++idx;
    }
    network->nodes[0]->muddle->ConnectTo(desired_peers.back());
    ASSERT_EQ(desired_peers.size(), M);

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

    // Testing that we are connected to the desired
    idx        = 0;
    uint64_t j = M - 1;
    for (auto &n : network->nodes)
    {
      if ((idx % K) == 0)
      {
        std::cout << "Testing " << j << " for " << idx << " " << desired_peers.size() << std::endl;
        EXPECT_TRUE(n->muddle->IsConnectingOrConnected(desired_peers[j]));
        j = (j + 1) % M;
      }
      ++idx;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1400));

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

    // Waiting for the network to settle.
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // We then expect that we still have the same desired peers as before.
    idx        = 0;
    uint64_t j = M - 1;
    for (auto &n : network->nodes)
    {
      if ((idx % K) == 0)
      {
        EXPECT_TRUE(n->muddle->IsConnectingOrConnected(desired_peers[j]));
        j = (j + 1) % M;
      }
      ++idx;
    }

    network->Stop();
  }
  network->Shutdown();
}
