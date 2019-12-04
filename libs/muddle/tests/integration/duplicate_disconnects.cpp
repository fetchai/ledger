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