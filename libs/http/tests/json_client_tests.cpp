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

#include "http/json_client.hpp"
#include "http/http_client.hpp"
#include "http/https_client.hpp"

#include "gtest/gtest.h"

using fetch::http::JsonClient;
using fetch::http::HttpClient;
using fetch::http::HttpsClient;

TEST(JsonClientTests, CheckBuildHttp)
{
  JsonClient client{JsonClient::CreateFromUrl("http://foo.bar.baz")};

  // try and access the
  auto const *http_client = dynamic_cast<HttpClient const *>(&client.underlying_client());
  ASSERT_NE(http_client, nullptr); // check
  EXPECT_EQ(http_client->host(), "foo.bar.baz");
  EXPECT_EQ(http_client->port(), 80);
}

TEST(JsonClientTests, CheckBuildHttpWithPort)
{
  JsonClient client{JsonClient::CreateFromUrl("http://baz.bar.foo:1234")};

  // try and access the
  auto const *http_client = dynamic_cast<HttpClient const *>(&client.underlying_client());
  ASSERT_NE(http_client, nullptr); // check
  EXPECT_EQ(http_client->host(), "baz.bar.foo");
  EXPECT_EQ(http_client->port(), 1234);
}

TEST(JsonClientTests, CheckBuildHttps)
{
  JsonClient client{JsonClient::CreateFromUrl("https://bar.bar.foo")};

  // try and access the
  auto const *https_client = dynamic_cast<HttpsClient const *>(&client.underlying_client());
  ASSERT_NE(https_client, nullptr); // check
  EXPECT_EQ(https_client->host(), "bar.bar.foo");
  EXPECT_EQ(https_client->port(), 443);

}

TEST(JsonClientTests, CheckBuildHttpsWithPort)
{
  JsonClient client{JsonClient::CreateFromUrl("https://foo.baz.bar:6543")};

  // try and access the
  auto const *https_client = dynamic_cast<HttpsClient const *>(&client.underlying_client());
  ASSERT_NE(https_client, nullptr); // check
  EXPECT_EQ(https_client->host(), "foo.baz.bar");
  EXPECT_EQ(https_client->port(), 6543);
}

TEST(JsonClientTests, CheckInvalidUrl)
{
  ASSERT_THROW(JsonClient::CreateFromUrl("fetch://foo.baz.bar:6543"), std::runtime_error);
}
