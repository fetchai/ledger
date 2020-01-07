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

#include "network/peer.hpp"
#include "network/uri.hpp"
#include "shards/manifest_entry.hpp"

#include "gtest/gtest.h"

using fetch::shards::ManifestEntry;
using fetch::network::Uri;
using fetch::network::Peer;
using fetch::muddle::Address;

TEST(ManifestEntryTests, CheckDefaultConstruction)
{
  ManifestEntry entry{};

  EXPECT_EQ(0, entry.local_port());
  EXPECT_EQ(0, entry.address().size());
  EXPECT_EQ(0, entry.uri().uri().size());
}

TEST(ManifestEntryTests, CheckPeerConstruction)
{
  Peer          peer{"127.0.0.1", 1234};
  ManifestEntry entry{peer};

  EXPECT_EQ(entry.local_port(), 1234);
  EXPECT_TRUE(entry.address().empty());
  ASSERT_TRUE(entry.uri().IsTcpPeer());
  EXPECT_EQ(entry.uri().GetTcpPeer(), peer);

  Address const address{"<muddle address>"};
  entry.UpdateAddress(address);
  EXPECT_EQ(entry.address(), address);
}

TEST(ManifestEntryTests, CheckUriConstruction)
{
  Uri           uri{"tcp://127.0.0.1:1234"};
  ManifestEntry entry{uri};

  EXPECT_EQ(entry.local_port(), 1234);
  EXPECT_TRUE(entry.address().empty());
  ASSERT_TRUE(entry.uri().IsTcpPeer());
  EXPECT_EQ(entry.uri(), uri);

  Address const address{"<muddle address>"};
  entry.UpdateAddress(address);
  EXPECT_EQ(entry.address(), address);
}

TEST(ManifestEntryTests, CheckUriWithLocalPortConstruction)
{
  Uri           uri{"tcp://127.0.0.1:1234"};
  ManifestEntry entry{uri, 4321};

  EXPECT_EQ(entry.local_port(), 4321);
  EXPECT_TRUE(entry.address().empty());
  ASSERT_TRUE(entry.uri().IsTcpPeer());
  EXPECT_EQ(entry.uri(), uri);

  Address const address{"<muddle address>"};
  entry.UpdateAddress(address);
  EXPECT_EQ(entry.address(), address);
}
