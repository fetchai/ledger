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

#include "shards/service_identifier.hpp"

#include "gtest/gtest.h"

using fetch::shards::ServiceIdentifier;

TEST(ServiceIdentifierTests, CheckDefaultConstruction)
{
  ServiceIdentifier id{};

  EXPECT_EQ(id.type(), ServiceIdentifier::Type::INVALID);
  EXPECT_EQ(id.instance(), uint32_t{ServiceIdentifier::SINGLETON_SERVICE});
  EXPECT_EQ(id.ToString(), "Invalid");
}

TEST(ServiceIdentifierTests, CheckCoreConstruction)
{
  ServiceIdentifier id{ServiceIdentifier::Type::CORE};

  EXPECT_EQ(id.type(), ServiceIdentifier::Type::CORE);
  EXPECT_EQ(id.instance(), uint32_t{ServiceIdentifier::SINGLETON_SERVICE});
  EXPECT_EQ(id.ToString(), "Core");
}

TEST(ServiceIdentifierTests, CheckHttpConstruction)
{
  ServiceIdentifier id{ServiceIdentifier::Type::HTTP};

  EXPECT_EQ(id.type(), ServiceIdentifier::Type::HTTP);
  EXPECT_EQ(id.instance(), uint32_t{ServiceIdentifier::SINGLETON_SERVICE});
  EXPECT_EQ(id.ToString(), "Http");
}

TEST(ServiceIdentifierTests, CheckDkgConstruction)
{
  ServiceIdentifier id{ServiceIdentifier::Type::DKG};

  EXPECT_EQ(id.type(), ServiceIdentifier::Type::DKG);
  EXPECT_EQ(id.instance(), uint32_t{ServiceIdentifier::SINGLETON_SERVICE});
  EXPECT_EQ(id.ToString(), "Dkg");
}

TEST(ServiceIdentifierTests, CheckLane0Construction)
{
  ServiceIdentifier id{ServiceIdentifier::Type::LANE, 0};

  EXPECT_EQ(id.type(), ServiceIdentifier::Type::LANE);
  EXPECT_EQ(id.instance(), 0);
  EXPECT_EQ(id.ToString(), "Lane/0");
}

TEST(ServiceIdentifierTests, CheckLane1Construction)
{
  ServiceIdentifier id{ServiceIdentifier::Type::LANE, 1};

  EXPECT_EQ(id.type(), ServiceIdentifier::Type::LANE);
  EXPECT_EQ(id.instance(), 1);
  EXPECT_EQ(id.ToString(), "Lane/1");
}

TEST(ServiceIdentifierTests, CheckEquality)
{
  ServiceIdentifier id1{ServiceIdentifier::Type::CORE};
  ServiceIdentifier id2{ServiceIdentifier::Type::DKG};

  EXPECT_FALSE(id1 == id2);

  id1 = id2;

  EXPECT_TRUE(id1 == id2);
}
