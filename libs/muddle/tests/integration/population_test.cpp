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
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "network_helpers.hpp"
#include "router.hpp"

#include "gtest/gtest.h"

TEST(RoutingTests, PopulationTest)
{

  // Creating network
  std::size_t N       = 10;
  auto        config  = fetch::muddle::TrackerConfiguration::AllOn();
  auto        network = Network::New(N, config);
  LinearConnectivity(network, std::chrono::seconds(5));

  // Waiting for initial settlement
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));

  // Killing 5 connections and adding 5 new ones
  for (uint64_t i = 0; i < 5; ++i)
  {
    // Adding new node
    network->AddNode(config);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Smashing longest living node
    network->PopFrontNode();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  // Giving the system time to settle
  std::this_thread::sleep_for(std::chrono::milliseconds(N * 1000));

  // We expect the total number of connections (in and out) that any one node has to be
  // be at least the maximum number of kademlia connections
  std::unordered_set<fetch::muddle::Address> all_addresses1;
  std::unordered_set<fetch::muddle::Address> all_addresses2;
  uint64_t                                   q = 0;
  for (auto &n : network->nodes)
  {
    // Waiting up to 40 seconds for the connections to come around
    while ((n->muddle->GetNumDirectlyConnectedPeers() < config.max_kademlia_connections) &&
           (q < 100))
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(400));
      ++q;
    }
    EXPECT_GE(n->muddle->GetNumDirectlyConnectedPeers(), config.max_kademlia_connections);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  // Checkiong that all peers are in the network
  for (auto &n : network->nodes)
  {
    for (auto const &adr : n->muddle->GetDirectlyConnectedPeers())
    {
      all_addresses1.emplace(adr);
    }
    all_addresses2.emplace(n->address);
  }

  EXPECT_GE(all_addresses1.size(), N);
  for (auto &adr : all_addresses2)
  {
    EXPECT_NE(all_addresses1.find(adr), all_addresses1.end());
  }

  network->Stop();
  network->Shutdown();
}
