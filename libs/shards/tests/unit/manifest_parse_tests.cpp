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

#include "network/uri.hpp"
#include "shards/manifest.hpp"
#include "shards/service_identifier.hpp"

#include "gtest/gtest.h"

namespace {

using fetch::network::Uri;
using fetch::shards::ServiceIdentifier;
using fetch::shards::Manifest;

TEST(ManifestParseTests, CheckFullManifestParse)
{
  static const char *FULL_MANIFEST = R"(
{
    "http": { "uri": "tcp://127.0.0.1:8000", "port": 50000},
    "p2p": { "uri": "tcp://127.0.0.1:8001", "port": 50001},
    "dkg": { "uri": "tcp://127.0.0.1:8002", "port": 50002},
    "lanes": [
        { "uri": "tcp://127.0.0.1:8010", "port": 50010},
        { "uri": "tcp://127.0.0.1:8012", "port": 50012}
    ]
}
)";

  Manifest manifest{};
  ASSERT_TRUE(manifest.Parse(FULL_MANIFEST));

  // look up http module
  auto it = manifest.FindService(ServiceIdentifier::Type::HTTP);
  ASSERT_FALSE(it == manifest.end());

  EXPECT_EQ(it->second.uri(), Uri{"tcp://127.0.0.1:8000"});
  EXPECT_EQ(it->second.local_port(), 50000);

  // look up core module
  it = manifest.FindService(ServiceIdentifier::Type::CORE);
  ASSERT_FALSE(it == manifest.end());

  EXPECT_EQ(it->second.uri(), Uri{"tcp://127.0.0.1:8001"});
  EXPECT_EQ(it->second.local_port(), 50001);

  // look up dkg module
  it = manifest.FindService(ServiceIdentifier::Type::DKG);
  ASSERT_FALSE(it == manifest.end());

  EXPECT_EQ(it->second.uri(), Uri{"tcp://127.0.0.1:8002"});
  EXPECT_EQ(it->second.local_port(), 50002);

  // look up lane 0 module
  it = manifest.FindService(ServiceIdentifier{ServiceIdentifier::Type::LANE, 0});
  ASSERT_FALSE(it == manifest.end());

  EXPECT_EQ(it->second.uri(), Uri{"tcp://127.0.0.1:8010"});
  EXPECT_EQ(it->second.local_port(), 50010);

  // look up lane 1 module
  it = manifest.FindService(ServiceIdentifier{ServiceIdentifier::Type::LANE, 1});
  ASSERT_FALSE(it == manifest.end());

  EXPECT_EQ(it->second.uri(), Uri{"tcp://127.0.0.1:8012"});
  EXPECT_EQ(it->second.local_port(), 50012);
}

}  // namespace
