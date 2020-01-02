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

#include "core/string/ends_with.hpp"
#include "core/string/replace.hpp"
#include "core/string/starts_with.hpp"
#include "core/string/to_lower.hpp"
#include "core/string/trim.hpp"

#include "gtest/gtest.h"

#include <string>

namespace {

using namespace fetch::core;
using namespace fetch::string;

std::string Escape(std::string const &s)
{
  std::string::size_type i0{};
  std::string            ret_val{'"'};
  for (auto i = s.find_first_of("\n\t"); i != std::string::npos;
       i0 = i + 1, i = s.find_first_of("\n\t", i + 1))
  {
    ret_val.append(s, i0, i - i0) += '\\';
    ret_val += (s[i] == '\n' ? 'n' : 't');
  }
  return {std::move(ret_val.append(s, i0) += '"')};
}

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

TEST(StringTests, check_Replace)
{
  auto s = Replace("Space shuttle ready to start", 's', 'z');
  EXPECT_EQ(s, "Space zhuttle ready to ztart")
      << "Replace(\"Space shuttle ready to start\", 's', 'z')";

  s = Replace("Space shuttle ready to start", 'z', 'm');
  EXPECT_EQ(s, "Space shuttle ready to start")
      << "Replace(\"Space shuttle ready to start\", 'z', 'm')";
}

TEST(StringTests, check_TrimFromLeft_removes_leading_whitespace)
{
  for (std::string s : {"    1234", " \t \n 1234", "1234"})
  {
    TrimFromLeft(s);
    EXPECT_EQ(s, "1234") << Escape(s);
  }

  std::string s = "    ";
  TrimFromLeft(s);
  EXPECT_EQ(s, "");

  s = " \t \n ";
  TrimFromLeft(s);
  EXPECT_EQ(s, "");
}

TEST(StringTests, check_TrimFromLeft_does_not_remove_trailing_whitespace)
{
  for (std::string s : {"    1234 \t \n ", " \t \n 1234 \t \n ", "1234 \t \n "})
  {
    TrimFromLeft(s);
    EXPECT_EQ(s, "1234 \t \n ") << Escape(s);
  }
}

TEST(StringTests, check_TrimFromRight_removes_trailing_whitespace)
{
  for (std::string s : {"1234    ", "1234 \t \n ", "1234"})
  {
    TrimFromRight(s);
    EXPECT_EQ(s, "1234") << Escape(s);
  }

  std::string s = "    ";
  TrimFromRight(s);
  EXPECT_EQ(s, "");

  s = " \t \n ";
  TrimFromRight(s);
  EXPECT_EQ(s, "");
}

TEST(StringTests, check_TrimFromRight_does_not_remove_leading_whitespace)
{
  for (std::string s : {" \t \n 1234    ", " \t \n 1234 \t \n ", " \t \n 1234"})
  {
    TrimFromRight(s);
    EXPECT_EQ(s, " \t \n 1234") << Escape(s);
  }
}

TEST(StringTests, check_Trim)
{
  for (std::string s : {"1234    ", "1234 \t \n ", "    1234", " \t \n 1234", "    1234     ",
                        " \t \n 1234     ", "    1234 \t \n  ", " \t \n 1234 \t \n  ", "1234"})
  {
    Trim(s);
    EXPECT_EQ(s, "1234");
  }

  std::string s = "    ";
  Trim(s);
  EXPECT_EQ(s, "");

  s = " \t \n ";
  Trim(s);
  EXPECT_EQ(s, "");
}

TEST(StringTests, check_ToLower)
{
  std::string s = "Hi there!";
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
