#include <cstdlib>
#include <iostream>
#include <memory>

#include "network/p2pservice/p2ptrust.hpp"
#include <gtest/gtest.h>

using namespace fetch::p2p;

TEST(TrustTests, TrustGoesUp)
{
  P2PTrust<std::string> trust;
  trust.AddFeedback("peer1",
                    fetch::byte_array::ConstByteArray(),
                    fetch::p2p::BLOCK,
                    fetch::p2p::NEW_INFORMATION
                    );
  EXPECT_EQ(trust.IsPeerTrusted("peer1"), true);
}

TEST(TrustTests, TrustGoesDown)
{
  P2PTrust<std::string> trust;
  trust.AddFeedback("peer1",
                    fetch::byte_array::ConstByteArray(),
                    fetch::p2p::BLOCK,
                    fetch::p2p::LIED
                    );
  EXPECT_EQ(trust.IsPeerTrusted("peer1"), false);
}
