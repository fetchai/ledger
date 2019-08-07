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
#include "core/string/join.hpp"
#include "core/string/split.hpp"
#include "core/string/starts_with.hpp"

#include "gtest/gtest.h"

#include <string>

using namespace fetch::core;
using namespace testing;

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

class SplitTests : public Test
{
};

TEST_F(SplitTests, split_returns_a_vector_of_string_segments)
{
  std::string const              input    = "api/tx/0a9b";
  std::vector<std::string> const expected = {"api", "tx", "0a9b"};

  EXPECT_EQ(Split(input, "/"), expected);
}

TEST_F(SplitTests, leading_separator_results_in_initial_empty_segment)
{
  std::string const              input    = "/api/tx/0a9b";
  std::vector<std::string> const expected = {"", "api", "tx", "0a9b"};

  EXPECT_EQ(Split(input, "/"), expected);
}

TEST_F(SplitTests, trailing_separator_results_in_terminal_empty_segment)
{
  std::string const              input    = "api/tx/0a9b/";
  std::vector<std::string> const expected = {"api", "tx", "0a9b", ""};

  EXPECT_EQ(Split(input, "/"), expected);
}

TEST_F(SplitTests, if_separator_is_empty_input_string_is_returned_as_one_segment)
{
  std::string const              input    = "api/tx/0a9b";
  std::vector<std::string> const expected = {input};

  EXPECT_EQ(Split(input, ""), expected);
}

TEST_F(SplitTests, if_separator_is_absent_input_string_is_returned_as_one_segment)
{
  std::string const              input    = "api/tx/0a9b";
  std::vector<std::string> const expected = {input};

  EXPECT_EQ(Split(input, "-"), expected);
}

TEST_F(SplitTests, input_of_N_separators_results_in_N_plus_1_empty_segments)
{
  std::string const              input    = "===";
  std::vector<std::string> const expected = {"", "", "", ""};

  EXPECT_EQ(Split(input, "="), expected);
}

TEST_F(SplitTests, multicharacter_separators_are_supported)
{
  std::string const              input    = "aaa---bbb---z";
  std::vector<std::string> const expected = {"aa", "bbb---z"};

  EXPECT_EQ(Split(input, "a---"), expected);
}

class JoinTests : public Test
{
};

TEST_F(JoinTests, join_returns_a_string_of_segments_connected_by_struts)
{
  std::vector<std::string> const input    = {"api", "tx", "0a9b"};
  std::string const              expected = "api/tx/0a9b";

  EXPECT_EQ(Join(input, "/"), expected);
}

TEST_F(JoinTests, initial_empty_segment_results_in_leading_strut)
{
  std::vector<std::string> const input    = {"", "api", "tx", "0a9b"};
  std::string const              expected = "/api/tx/0a9b";

  EXPECT_EQ(Join(input, "/"), expected);
}

TEST_F(JoinTests, terminal_empty_segment_results_in_trailing_strut)
{
  std::vector<std::string> const input    = {"api", "tx", "0a9b", ""};
  std::string const              expected = "api/tx/0a9b/";

  EXPECT_EQ(Join(input, "/"), expected);
}

TEST_F(JoinTests, if_input_is_empty_Join_returns_empty_string)
{
  std::vector<std::string> const input = {};

  EXPECT_EQ(Join(input, "/"), "");
}

TEST_F(JoinTests, if_input_has_one_element_it_is_returned_unchanged)
{
  std::vector<std::string> const input    = {"xyz"};
  std::string const              expected = input.front();

  EXPECT_EQ(Join(input, "/"), expected);
}

TEST_F(JoinTests, multicharacter_struts_are_supported)
{
  std::vector<std::string> const input    = {"aaa", "bbb", "ccc"};
  std::string const              expected = "aaa---bbb---ccc";

  EXPECT_EQ(Join(input, "---"), expected);
}

TEST_F(JoinTests, empty_struts_are_supported)
{
  std::vector<std::string> const input    = {"aaa", "bbb", "ccc"};
  std::string const              expected = "aaabbbccc";

  EXPECT_EQ(Join(input, ""), expected);
}

}  // namespace
