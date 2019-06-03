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

#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

namespace {

class StringTests : public ::testing::Test
{
public:
  VmTestToolkit toolkit;
};

TEST_F(StringTests, length_returns_number_of_characters)
{
  static char const *TEXT = R"(
    function main()
      var output = Array<Int32>(2);
      output[0] = 'abc'.length();
      output[1] = 'abc def gh'.length();

      print(output);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[3, 10]");
}

TEST_F(StringTests, length_returns_zero_for_empty_string)
{
  static char const *TEXT = R"(
    function main()
      print(''.length());
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "0");
}

TEST_F(StringTests, trim_removes_leading_whitespace)
{
  static char const *TEXT = R"(
    function main()
      var text = '   abc def';
      text.trim();
      print(text);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "abc def");
}

TEST_F(StringTests, trim_removes_trailing_whitespace)
{
  static char const *TEXT = R"(
    function main()
      var text = 'abc def  ';
      text.trim();
      print(text);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "abc def");
}

TEST_F(StringTests, trim_removes_both_leading_and_trailing_whitespace)
{
  static char const *TEXT = R"(
    function main()
      var text = '   abc def  ';
      text.trim();
      print(text);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "abc def");
}

TEST_F(StringTests, trim_is_noop_if_string_has_no_leading_or_trailing_whitespace)
{
  static char const *TEXT = R"(
    function main()
      var text = 'abc def';
      text.trim();
      print(text);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "abc def");
}

TEST_F(StringTests, trim_is_noop_if_string_is_empty)
{
  static char const *TEXT = R"(
    function main()
      var text = '';
      text.trim();
      print(text);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "");
}

TEST_F(StringTests, trim_leaves_string_empty_if_it_contains_only_whitespace)
{
  static char const *TEXT = R"(
    function main()
      var text = '   ';
      text.trim();
      print(text);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "");
}

TEST_F(StringTests, find_returns_zero_based_index_of_first_occurrence_of_substring_in_string)
{
  static char const *TEXT = R"(
    function main()
      var text = 'ab bbc';

var output = Array<Int32>(3);
      output[0] = text.find('ab');
      output[1] = text.find('bb');
      output[2] = text.find('c');

      print(output);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[0, 3, 5]");
}

TEST_F(StringTests, find_returns_minus_one_if_substring_not_found)
{
  static char const *TEXT = R"(
    function main()
      print('abc'.find('x'));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "-1");
}

TEST_F(StringTests, find_returns_minus_one_if_string_is_empty)
{
  static char const *TEXT = R"(
    function main()
      print(''.find('abc'));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "-1");
}

TEST_F(StringTests, find_returns_minus_one_if_substring_is_empty)
{
  static char const *TEXT = R"(
    function main()
      print('abc'.find(''));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "-1");
}

TEST_F(StringTests, find_returns_minus_one_if_both_string_and_substring_are_empty)
{
  static char const *TEXT = R"(
    function main()
      print(''.find(''));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "-1");
}

TEST_F(StringTests, reverse_changes_string_contents_to_the_original_characters_but_in_reverse_order)
{
  static char const *TEXT = R"(
    function main()
      var text = 'xyz';
      text.reverse();
      print(text);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "zyx");
}

TEST_F(StringTests, reverse_is_noop_if_string_is_empty)
{
  static char const *TEXT = R"(
    function main()
      var text = '';
      text.reverse();
      print(text);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "");
}

TEST_F(
    StringTests,
    given_the_zero_based_start_and_end_index_substring_returns_a_new_string_excluding_the_end_character)
{
  static char const *TEXT = R"(
    function main()
      print('abcdef'.substr(1, 3));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "bc");
}

TEST_F(StringTests, substring_returns_empty_string_if_start_and_end_are_equal)
{
  static char const *TEXT = R"(
    function main()
      var text = 'abcdef';
      print(text.substr(0, 0));
      print(text.substr(1, 1));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "");
}

TEST_F(StringTests, substring_returns_the_whole_string_given_zero_and_length)
{
  static char const *TEXT = R"(
    function main()
      print('abcdef'.substr(0, 6));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "abcdef");
}

TEST_F(StringTests, substring_fails_if_start_is_negative)
{
  static char const *TEXT = R"(
    function main()
      print('abcdef'.substr(-1, 1));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(StringTests, substring_fails_if_end_is_greater_than_length)
{
  static char const *TEXT = R"(
    function main()
      print('abcdef'.substr(0, 10000));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(StringTests, substring_fails_if_start_is_greater_than_end)
{
  static char const *TEXT = R"(
    function main()
      'abcdef'.substr(3, 2);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

}  // namespace
