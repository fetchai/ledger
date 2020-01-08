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

#include "gtest/gtest.h"
#include "oef-base/utils/Uri.hpp"

class OefBaseUtilsTests : public testing::Test
{
};

TEST_F(OefBaseUtilsTests, UriTest)
{
  Uri uri("tcp://127.0.0.1:10000");

  EXPECT_EQ(uri.proto, "tcp");
  EXPECT_EQ(uri.host, "127.0.0.1");
  EXPECT_EQ(uri.port, 10000);
  EXPECT_EQ(uri.path, "");
}

TEST_F(OefBaseUtilsTests, UriTest2)
{
  Uri uri("127.0.0.1:10000");
  uri.diagnostic();

  EXPECT_EQ(uri.proto, "");
  EXPECT_EQ(uri.host, "127.0.0.1");
  EXPECT_EQ(uri.port, 10000);
  EXPECT_EQ(uri.path, "");
}
