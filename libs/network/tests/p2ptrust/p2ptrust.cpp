#include <cstdlib>
#include <iostream>
#include <memory>

#include "network/p2pservice/p2ptrust.hpp"
#include <gtest/gtest.h>

using namespace fetch::p2p;
using fetch::byte_array::ConstByteArray;


TEST(TrustTests, TrustGoesUp)
{
  P2PTrust<std::string> trust;
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::NEW_INFORMATION);
  EXPECT_EQ(trust.IsPeerTrusted("peer1"), true);
}

TEST(TrustTests, TrustGoesDown)
{
  P2PTrust<std::string> trust;
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    TrustQuality::LIED);
  EXPECT_EQ(trust.IsPeerTrusted("peer1"), false);
}

TEST(TrustTests, TrustGoesWayDown)
{
  P2PTrust<std::string>    trust;
  TrustQuality qual = TrustQuality::LIED;
  for (int i = 0; i < 20; i++)
    if (i & 1)
    {
      qual = TrustQuality::DUPLICATE;
    }
    else
    {
      qual = TrustQuality::NEW_INFORMATION;
    }
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK, qual);
  EXPECT_EQ(trust.IsPeerTrusted("peer1"), true);

  for (int i = 0; i < 5; i++)
  {
    trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::PEER,
                      TrustQuality::LIED);
  }

  EXPECT_EQ(trust.IsPeerTrusted("peer1"), false);  // You **WOULDN'T LET IT LIE**... </vic_reeves>
}
