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
#include "network_helpers.hpp"
#include "router.hpp"

#include "gtest/gtest.h"

TEST(RoutingTests, DuplicateConnections)
{
  // Creating network
  auto network = Network::New(10, fetch::muddle::TrackerConfiguration::AllOn());
  AllToAllConnectivity(network, std::chrono::duration_cast<fetch::muddle::Muddle::Duration>(
                                    std::chrono::milliseconds(10 * 2000)));

  //  std::size_t total_count = 0;

  // Waiting for the network to settle.
  std::this_thread::sleep_for(std::chrono::milliseconds(10 * 2000));
}
