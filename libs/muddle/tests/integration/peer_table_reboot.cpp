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
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "network_helpers.hpp"
#include "router.hpp"

#include "gtest/gtest.h"

TEST(RoutingTests, PeerTestReboot)
{
  {
    // Creating network
    std::size_t N       = 10;
    auto        network = Network::New(N, fetch::muddle::TrackerConfiguration::AllOn());
    uint64_t    idx     = 0;
    for (auto &n : network->nodes)
    {
      n->muddle->SetPeerTableFile("peer_table" + std::to_string(idx) + ".cache");
      ++idx;
    }

    LinearConnectivity(network, std::chrono::seconds(5));

    std::this_thread::sleep_for(std::chrono::milliseconds(N * 2000));
    network->Stop();
  }

  // Restart
  {
  }
}
