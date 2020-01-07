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
#include "shards/manifest_entry.hpp"
#include "shards/service_identifier.hpp"

#include "gtest/gtest.h"

#include <sstream>

namespace {

using fetch::network::Uri;
using fetch::shards::Manifest;
using fetch::shards::ManifestEntry;
using fetch::shards::ServiceIdentifier;

struct ManifestItem
{
  ServiceIdentifier id{};
  ManifestEntry     entry{};
};

ManifestItem CreateEntry(std::string const &uri, ServiceIdentifier::Type type)
{
  ManifestItem item{ServiceIdentifier{type}, ManifestEntry{Uri{uri}}};

  // generate a fake "muddle address"
  std::ostringstream oss;
  oss << '<' << item.id.ToString() << " Muddle Address>";
  item.entry.UpdateAddress(oss.str());

  return item;
}

class ManifestTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    auto const http = CreateEntry("tcp://127.0.0.1:8000", ServiceIdentifier::Type::HTTP);
    auto const core = CreateEntry("tcp://127.0.0.1:8001", ServiceIdentifier::Type::CORE);

    // add the services to the manifest
    manifest_.AddService(http.id, http.entry);
    manifest_.AddService(core.id, core.entry);
  }

  Manifest manifest_;
};

TEST_F(ManifestTests, CheckDefaultConstruction)
{
  Manifest default_manifest{};
  EXPECT_EQ(0, default_manifest.size());
}

TEST_F(ManifestTests, CheckFindCoreSerivceById)
{
  auto it = manifest_.FindService(ServiceIdentifier{ServiceIdentifier::Type::CORE});
  ASSERT_TRUE(it != manifest_.end());

  EXPECT_EQ(it->first.type(), ServiceIdentifier::Type::CORE);
  EXPECT_EQ(it->second.uri(), Uri{"tcp://127.0.0.1:8001"});
  EXPECT_EQ(it->second.address(), "<Core Muddle Address>");
}

TEST_F(ManifestTests, CheckFindHttpSerivceById)
{
  auto it = manifest_.FindService(ServiceIdentifier{ServiceIdentifier::Type::HTTP});
  ASSERT_TRUE(it != manifest_.end());

  EXPECT_EQ(it->first.type(), ServiceIdentifier::Type::HTTP);
  EXPECT_EQ(it->second.uri(), Uri{"tcp://127.0.0.1:8000"});
  EXPECT_EQ(it->second.address(), "<Http Muddle Address>");
}

TEST_F(ManifestTests, CheckNotPresentServiceById)
{
  auto it = manifest_.FindService(ServiceIdentifier{ServiceIdentifier::Type::DKG});
  ASSERT_TRUE(it == manifest_.end());
}

TEST_F(ManifestTests, CheckFindCoreSerivceByType)
{
  auto it = manifest_.FindService(ServiceIdentifier::Type::CORE);
  ASSERT_TRUE(it != manifest_.end());

  EXPECT_EQ(it->first.type(), ServiceIdentifier::Type::CORE);
  EXPECT_EQ(it->second.uri(), Uri{"tcp://127.0.0.1:8001"});
  EXPECT_EQ(it->second.address(), "<Core Muddle Address>");
}

TEST_F(ManifestTests, CheckFindHttpSerivceByType)
{
  auto it = manifest_.FindService(ServiceIdentifier::Type::HTTP);
  ASSERT_TRUE(it != manifest_.end());

  EXPECT_EQ(it->first.type(), ServiceIdentifier::Type::HTTP);
  EXPECT_EQ(it->second.uri(), Uri{"tcp://127.0.0.1:8000"});
  EXPECT_EQ(it->second.address(), "<Http Muddle Address>");
}

TEST_F(ManifestTests, CheckNotPresentServiceByType)
{
  auto it = manifest_.FindService(ServiceIdentifier::Type::DKG);
  ASSERT_TRUE(it == manifest_.end());
}

}  // namespace
