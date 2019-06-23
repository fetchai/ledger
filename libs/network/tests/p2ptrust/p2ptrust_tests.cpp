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

#include "network/p2pservice/p2ptrust.hpp"

#include "gtest/gtest.h"

#include <cstdlib>
#include <memory>

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
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK, TrustQuality::LIED);
  EXPECT_EQ(trust.IsPeerTrusted("peer1"), false);
}

TEST(TrustTests, TrustGoesWayDown)
{
  P2PTrust<std::string> trust;
  TrustQuality          qual = TrustQuality::LIED;
  for (int i = 0; i < 20; i++)
  {
    if (i & 1)
    {
      qual = TrustQuality::DUPLICATE;
    }
    else
    {
      qual = TrustQuality::NEW_INFORMATION;
    }
  }
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK, qual);
  EXPECT_EQ(trust.IsPeerTrusted("peer1"), true);

  for (int i = 0; i < 5; i++)
  {
    trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::PEER, TrustQuality::LIED);
  }

  EXPECT_EQ(trust.IsPeerTrusted("peer1"), false);
}
