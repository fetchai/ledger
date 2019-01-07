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

#include <cstdlib>
#include <iostream>
#include <memory>

#include "network/p2pservice/p2ptrust_bayrank.hpp"
#include <gtest/gtest.h>

using namespace fetch::p2p;
using fetch::byte_array::ConstByteArray;
using Gaussian = fetch::math::statistics::Gaussian<double>;

template <typename IDENTITY>
class P2PTrustBayRankExtendedForTest : public P2PTrustBayRank<IDENTITY>
{
public:
  // Construction / Destruction
  P2PTrustBayRankExtendedForTest()                                          = default;
  P2PTrustBayRankExtendedForTest(const P2PTrustBayRankExtendedForTest &rhs) = delete;
  P2PTrustBayRankExtendedForTest(P2PTrustBayRankExtendedForTest &&rhs)      = delete;
  ~P2PTrustBayRankExtendedForTest() override                                = default;

  // Operators
  P2PTrustBayRankExtendedForTest operator=(const P2PTrustBayRankExtendedForTest &rhs) = delete;
  P2PTrustBayRankExtendedForTest operator=(P2PTrustBayRankExtendedForTest &&rhs) = delete;

  Gaussian GetGaussianOfPeer(IDENTITY const &peer_ident)
  {
    FETCH_LOCK(this->mutex_);
    auto ranking_it = this->ranking_store_.find(peer_ident);
    if (ranking_it != this->ranking_store_.end())
    {
      if (ranking_it->second < this->trust_store_.size())
      {
        return this->trust_store_[ranking_it->second].g;
      }
    }
    return Gaussian();
  }
};

TEST(TrustTests, BayNewInfo)
{
  P2PTrustBayRankExtendedForTest<std::string> trust;
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::NEW_INFORMATION);
  auto g  = trust.GetGaussianOfPeer("peer1");
  auto rg = LookupReferencePlayer(TrustQuality::NEW_INFORMATION);
  EXPECT_EQ(g.mu() > rg.mu(), true);
  EXPECT_EQ(g.sigma() < rg.sigma(), true);
  EXPECT_EQ(trust.IsPeerTrusted("peer1"), true);
}

TEST(TrustTests, BayBadInfo)
{
  P2PTrustBayRankExtendedForTest<std::string> trust;

  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK, fetch::p2p::TrustQuality::LIED);

  auto rg = LookupReferencePlayer(TrustQuality::NEW_INFORMATION);
  auto g  = trust.GetGaussianOfPeer("peer1");
  EXPECT_EQ(g.mu() < rg.mu(), true);
  EXPECT_EQ(g.sigma() < rg.sigma(), true);

  EXPECT_EQ(trust.IsPeerTrusted("peer1"), false);

  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::NEW_INFORMATION);

  EXPECT_EQ(trust.IsPeerTrusted("peer1"), false);

  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::NEW_INFORMATION);

  EXPECT_EQ(trust.IsPeerTrusted("peer1"), true);
}

TEST(TrustTests, BayBadConnection)
{
  P2PTrustBayRankExtendedForTest<std::string> trust;

  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::BAD_CONNECTION);
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::BAD_CONNECTION);
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::BAD_CONNECTION);
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::BAD_CONNECTION);
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::BAD_CONNECTION);
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::BAD_CONNECTION);
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::BAD_CONNECTION);

  EXPECT_EQ(trust.IsPeerTrusted("peer1"), true);

  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::BAD_CONNECTION);

  EXPECT_EQ(trust.IsPeerTrusted("peer1"), false);

  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::BAD_CONNECTION);
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::BAD_CONNECTION);
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::BAD_CONNECTION);
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::BAD_CONNECTION);

  EXPECT_EQ(trust.IsPeerTrusted("peer1"), false);

  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::NEW_INFORMATION);

  EXPECT_EQ(trust.IsPeerTrusted("peer1"), true);
}

TEST(TrustTests, BayDuplicate)
{
  P2PTrustBayRank<std::string> trust;
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::DUPLICATE);
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::DUPLICATE);
  trust.AddFeedback("peer1", ConstByteArray{}, TrustSubject::BLOCK,
                    fetch::p2p::TrustQuality::DUPLICATE);
  EXPECT_EQ(trust.IsPeerTrusted("peer1"), true);
}