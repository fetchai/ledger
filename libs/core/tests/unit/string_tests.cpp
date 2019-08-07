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

#include "core/string/ends_with.hpp"
#include "core/string/replace.hpp"
#include "core/string/starts_with.hpp"
#include "core/string/to_lower.hpp"
#include "core/string/trim.hpp"

#include "gtest/gtest.h"

using namespace fetch::core;

template <class T, class U>
inline bool Equal(T &&t, U &&u)
{
  return t == u;
}

#define EXPECT_EQUAL(...) EXPECT_TRUE(Equal(__VA_ARGS__))

namespace {

TEST(StringTests, check_EndsWith)
{
  EXPECT_TRUE(EndsWith("Hello World", "Hello World"));
  EXPECT_TRUE(EndsWith("Hello World", "World"));
  EXPECT_FALSE(EndsWith("Hello World", "World2"));
  EXPECT_FALSE(EndsWith("Hello World", "2World"));
  EXPECT_TRUE(EndsWith("Hello World", ""));
  EXPECT_FALSE(EndsWith("Hello World", "o"));
  EXPECT_FALSE(EndsWith("Hello World", "Hello"));
  EXPECT_FALSE(EndsWith("Hello World", "Hello World..."));
  EXPECT_FALSE(EndsWith("Hello World", "...Hello World"));
}

TEST(StringTests, check_StartsWith)
{
  EXPECT_TRUE(StartsWith("Hello World", "Hello World"));
  EXPECT_TRUE(StartsWith("Hello World", "Hello"));
  EXPECT_FALSE(StartsWith("Hello World", "Hello2"));
  EXPECT_FALSE(StartsWith("Hello World", "2Hello"));
  EXPECT_TRUE(StartsWith("Hello World", ""));
  EXPECT_FALSE(StartsWith("Hello World", "o"));
  EXPECT_FALSE(StartsWith("Hello World", "World"));
  EXPECT_FALSE(StartsWith("Hello World", "Hello World..."));
  EXPECT_FALSE(StartsWith("Hello World", "...Hello World"));
}

using namespace fetch::string;

TEST(StringTests, check_Replace)
{
  EXPECT_EQUAL(Replace("Space shuttle ready to start", 's', 'z'), "Space zhuttle ready to ztart");
  EXPECT_EQUAL(Replace("Space shuttle ready to start", 'z', 'm'), "Space shuttle ready to start");
}

TEST(StringTests, check_Trim)
{
  std::string s;

  // TrimFromRight
  s = "    1234";
  TrimFromRight(s);
  EXPECT_EQ(s, "1234");

  s = "1234";
  TrimFromRight(s);
  EXPECT_EQ(s, "1234");

  s = "    ";
  TrimFromRight(s);
  EXPECT_EQ(s, "");

  // TrimFromLeft
  s = "1234    ";
  TrimFromLeft(s);
  EXPECT_EQ(s, "1234");

  s = "1234";
  TrimFromLeft(s);
  EXPECT_EQ(s, "1234");

  s = "    ";
  TrimFromLeft(s);
  EXPECT_EQ(s, "");

  // Trim
  s = "1234    ";
  Trim(s);
  EXPECT_EQ(s, "1234");

  s = "    1234";
  Trim(s);
  EXPECT_EQ(s, "1234");

  s = "    1234     ";
  Trim(s);
  EXPECT_EQ(s, "1234");

  s = "1234";
  Trim(s);
  EXPECT_EQ(s, "1234");

  s = "    ";
  Trim(s);
  EXPECT_EQ(s, "");
}

TEST(StringTests, check_ToLower)
{
  std::string s;

  s = "Hi there!";
  ToLower(s);
  EXPECT_EQ(s, "hi there!");

  s = "I SAID HI THERE!!!1111";
  ToLower(s);
  EXPECT_EQ(s, "i said hi there!!!1111");

  s = "oh, well, okay...";
  ToLower(s);
  EXPECT_EQ(s, "oh, well, okay...");

  s = "12345";
  ToLower(s);
  EXPECT_EQ(s, "12345");
}

}  // namespace
