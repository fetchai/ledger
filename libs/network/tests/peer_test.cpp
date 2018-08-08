#include "network/peer.hpp"

#include <gtest/gtest.h>

using fetch::network::Peer;

TEST(NetworkPeerTests, CheckBasicConstruction)
{
  Peer peer("localhost", 9090);

  EXPECT_EQ(peer.address(), "localhost");
  EXPECT_EQ(peer.port(), 9090);
}

TEST(NetworkPeerTests, CheckParseConstruction)
{
  Peer peer("localhost:9090");

  EXPECT_EQ(peer.address(), "localhost");
  EXPECT_EQ(peer.port(), 9090);
}