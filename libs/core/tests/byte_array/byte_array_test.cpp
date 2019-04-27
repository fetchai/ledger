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

#include "core/byte_array/byte_array.hpp"

#include "gtest/gtest.h"
//#include "gmock/gmock.h"

#include <iostream>
#include <set>
#include <unordered_set>

namespace fetch {
namespace byte_array {

namespace {

TEST(ByteArrayTest, test_replace)
{
  ByteArray arr("hello kitty, how are you?");

  //* PRODUCTION CODE UNDER TEST
  std::size_t const num_of_replacements = arr.Replace(' ', '-');

  //* EXPECTARTIONS
  ConstByteArray const expected_result("hello-kitty,-how-are-you?");
  EXPECT_EQ(expected_result, arr);
  EXPECT_EQ(4, num_of_replacements);
}

TEST(ByteArrayTest, std_set)
{
  {
    ConstByteArray           req{"hello kitty, how are you?"};
    std::set<ConstByteArray> set{req};
    EXPECT_EQ(set.size(), 1);
    ConstByteArray resp{"i'm great, thank you sunshine!"};
    EXPECT_TRUE(set.insert(resp).second);
    EXPECT_EQ(set.size(), 2);
    std::set<ConstByteArray>::const_iterator i{set.find(req)};
    EXPECT_NE(i, set.cend());
    i = set.find(resp);
    EXPECT_NE(i, set.cend());
    EXPECT_FALSE(set.insert(req).second);
    EXPECT_EQ(set.size(), 2);
    i = set.cbegin();
    EXPECT_NE(i, set.cend());
    EXPECT_EQ(*i, req);
    ++i;
    EXPECT_NE(i, set.cend());
    EXPECT_EQ(*i, resp);
    ++i;
    EXPECT_EQ(i, set.cend());
    for (auto const &cba : set)
    {
      std::cout << cba << std::endl;
    }
    EXPECT_EQ(set.erase(req), 1);
    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(set.erase(req), 0);
    EXPECT_EQ(set.size(), 1);
  }
  {
    ByteArray           req{"hello kitty, how are you?"};
    std::set<ByteArray> set{req};
    EXPECT_EQ(set.size(), 1);
    ByteArray resp{"i'm great, thank you sunshine!"};
    EXPECT_TRUE(set.insert(resp).second);
    EXPECT_EQ(set.size(), 2);
    std::set<ByteArray>::const_iterator i{set.find(req)};
    EXPECT_NE(i, set.cend());
    i = set.find(resp);
    EXPECT_NE(i, set.cend());
    EXPECT_FALSE(set.insert(req).second);
    EXPECT_EQ(set.size(), 2);
    i = set.cbegin();
    EXPECT_NE(i, set.cend());
    EXPECT_EQ(*i, req);
    ++i;
    EXPECT_NE(i, set.cend());
    EXPECT_EQ(*i, resp);
    ++i;
    EXPECT_EQ(i, set.cend());
    for (auto const &cba : set)
    {
      std::cout << cba << std::endl;
    }
    EXPECT_EQ(set.erase(req), 1);
    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(set.erase(req), 0);
    EXPECT_EQ(set.size(), 1);
  }
}

}  // namespace

}  // namespace byte_array
}  // namespace fetch
