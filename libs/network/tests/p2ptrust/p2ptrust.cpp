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

TEST(TrustTests, TrustGoesWayDown)
{
  P2PTrust<std::string> trust;
  fetch::p2p::P2PTrustFeedbackQuality qual;
  for (int i=0;i<20;i++)
    if (i&1)
    {
      qual = fetch::p2p::DUPLICATE;
    }
    else
    {
      qual = fetch::p2p::NEW_INFORMATION;
    }
    trust.AddFeedback("peer1",
                      fetch::byte_array::ConstByteArray(),
                      fetch::p2p::BLOCK,
                      qual
                      );
  EXPECT_EQ(trust.IsPeerTrusted("peer1"), true);

  for (int i=0;i<5;i++)
  {
    trust.AddFeedback("peer1",
                      fetch::byte_array::ConstByteArray(),
                      fetch::p2p::PEER,
                      fetch::p2p::LIED
                      );
  }

  EXPECT_EQ(trust.IsPeerTrusted("peer1"), false); // You **WOULDN'T LET IT LIE**... </vic_reeves>
}
