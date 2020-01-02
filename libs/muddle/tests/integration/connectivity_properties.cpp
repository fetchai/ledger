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
#include "network_helpers.hpp"
#include "router.hpp"

#include "gtest/gtest.h"

TEST(RoutingTests, RemovingDuplicateConnections)
{
  // Creating network
  std::size_t N = 10;
  auto network  = Network::New(N, fetch::muddle::TrackerConfiguration::DefaultConfiguration());
  AllToAllConnectivity(network, std::chrono::seconds(5));

  // Waiting for the network to settle.
  std::this_thread::sleep_for(std::chrono::milliseconds(N * 1000));

  std::size_t total{0};
  std::size_t total_in{0};
  std::size_t total_out{0};

  for (auto const &node : network->nodes)
  {
    auto direct = node->muddle->GetDirectlyConnectedPeers().size();
    EXPECT_EQ(direct, N - 1);

    total_in += node->muddle->GetIncomingConnectedPeers().size();
    total_out += node->muddle->GetOutgoingConnectedPeers().size();
    total += direct;
  }

  network->Stop();

  // TODO(tfr): Fix
  EXPECT_EQ(total_out, N * (N - 1) / 2);
  EXPECT_EQ(total_in, N * (N - 1) / 2);
}

TEST(RoutingTests, NoRemoval)
{
  // Creating network
  std::size_t N                = 10;
  auto        config           = fetch::muddle::TrackerConfiguration::DefaultConfiguration();
  config.disconnect_duplicates = false;
  auto network                 = Network::New(N, config);
  AllToAllConnectivity(network, std::chrono::seconds(5));

  // Waiting for the network to settle.
  std::this_thread::sleep_for(std::chrono::milliseconds(N * 1000));

  std::size_t total{0};
  std::size_t total_in{0};
  std::size_t total_out{0};

  for (auto const &node : network->nodes)
  {
    auto direct = node->muddle->GetDirectlyConnectedPeers().size();
    EXPECT_EQ(direct, N - 1);

    total_in += node->muddle->GetIncomingConnectedPeers().size();
    total_out += node->muddle->GetOutgoingConnectedPeers().size();
    total += direct;
  }

  network->Stop();

  // TODO(tfr): Fix
  EXPECT_EQ(total_out, N * (N - 1));
  EXPECT_EQ(total_in, N * (N - 1));
}

TEST(RoutingTests, NoRemovalIncludingSelf)
{
  // Creating network
  std::size_t N                = 10;
  auto        config           = fetch::muddle::TrackerConfiguration::DefaultConfiguration();
  config.disconnect_duplicates = false;
  config.disconnect_from_self  = false;

  auto network = Network::New(N, config);
  AllToAllConnectivity(network, std::chrono::seconds(5));

  // Waiting for the network to settle.
  std::this_thread::sleep_for(std::chrono::milliseconds(N * 1000));

  std::size_t total{0};
  std::size_t total_in{0};
  std::size_t total_out{0};

  for (auto const &node : network->nodes)
  {
    auto direct = node->muddle->GetDirectlyConnectedPeers().size();
    EXPECT_EQ(direct, N);

    total_in += node->muddle->GetIncomingConnectedPeers().size();
    total_out += node->muddle->GetOutgoingConnectedPeers().size();
    total += direct;
  }

  network->Stop();

  // TODO(tfr): Fix
  EXPECT_EQ(total_out, N * N);
  EXPECT_EQ(total_in, N * N);
}
