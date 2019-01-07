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

#include "network/uri.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <memory>

using fetch::network::Uri;

struct TestCase
{
  char const *text;
  Uri::Scheme scheme;
  char const *authority;
  bool        success;
};

std::ostream &operator<<(std::ostream &s, Uri::Scheme scheme)
{
  switch (scheme)
  {
  case Uri::Scheme::Tcp:
    s << "Tcp";
    break;
  case Uri::Scheme::Muddle:
    s << "Muddle";
    break;
  default:
    s << "Unknown";
    break;
  }

  return s;
}

std::ostream &operator<<(std::ostream &s, TestCase const &config)
{
  s << config.text;
  return s;
}

static const TestCase TEST_CASES[] = {
    {"tcp://127.0.0.1:8000", Uri::Scheme::Tcp, "127.0.0.1:8000", true},
    {"tcp://hostname:8000", Uri::Scheme::Tcp, "hostname:8000", true},
    {"muddle://rOA3MfBt0DdRtZRSo/gBFP2aD/YQTsd9lOh/Oc/Pzchrzz1wfhTUMpf9z8cc1kRltUpdlWznGzwroO8/"
     "rbdPXA==",
     Uri::Scheme::Muddle,
     "rOA3MfBt0DdRtZRSo/gBFP2aD/YQTsd9lOh/Oc/Pzchrzz1wfhTUMpf9z8cc1kRltUpdlWznGzwroO8/rbdPXA==",
     true},
    {"tcp://foo:bar", Uri::Scheme::Muddle, "foo:bar", false},
    {"muddle://badIdentityName", Uri::Scheme::Muddle, "badIdentityName", false}};

class UriTests : public ::testing::TestWithParam<TestCase>
{
protected:
};

TEST_P(UriTests, CheckConstruction)
{
  using UriPtr           = std::unique_ptr<Uri>;
  TestCase const &config = GetParam();

  UriPtr uri;

  if (config.success)
  {
    // construct the object
    EXPECT_NO_THROW(uri = std::make_unique<Uri>(config.text));
    EXPECT_EQ(config.scheme, uri->scheme());
    EXPECT_EQ(std::string{config.authority}, static_cast<std::string>(uri->authority()));
  }
  else
  {
    // ensure that the correct exception is thrown
    EXPECT_THROW(uri = std::make_unique<Uri>(config.text), std::runtime_error);
  }
}

TEST_P(UriTests, CheckParsing)
{
  TestCase const &config = GetParam();

  Uri uri;

  EXPECT_EQ(config.success, uri.Parse(config.text));
  if (config.success)
  {
    EXPECT_EQ(config.scheme, uri.scheme());
    EXPECT_EQ(std::string{config.authority}, static_cast<std::string>(uri.authority()));
  }
}

INSTANTIATE_TEST_CASE_P(ParamBased, UriTests, testing::ValuesIn(TEST_CASES), );
