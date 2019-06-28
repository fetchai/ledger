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

#include <string>
#include <utility>

using namespace fetch::byte_array;

TEST(reference_byte_array_gtest, ensuring_subbyte_arrays_come_out_correctly)
{
  char const *base    = "hello world";
  std::string basecpp = base;
  ByteArray   str(base);
  EXPECT_EQ(str, base);
  EXPECT_EQ(str, basecpp);
  EXPECT_EQ(str.SubArray(0, 5), "hello");
  EXPECT_EQ(str.SubArray(0, 5), basecpp.substr(0, 5));

  EXPECT_EQ(str.SubArray(6, str.size() - 6), "world");
  EXPECT_EQ(str.SubArray(6, str.size() - 6), basecpp.substr(6, basecpp.size() - 6));
}

TEST(reference_byte_array_gtest, testing_that_referencing_persists_after_subbyte_arraying)
{
  char const *base    = "hello world";
  std::string basecpp = base;
  ByteArray   str(base);
  ByteArray   copy = str.SubArray(6, str.size() - 6);
  copy[0]          = 'k';
  copy[1]          = 'i';
  copy[2]          = 't';
  copy[3]          = 't';
  copy[4]          = 'y';

  EXPECT_EQ(str, "hello kitty");
}

TEST(reference_byte_array_gtest, testing_sub_array_of_sub_array)
{
  char const *base = "hello world";
  ByteArray   bc{base};
  ByteArray   sub_array_1 = bc.SubArray(bc.size() - 5, 5);
  sub_array_1[0]          = 'k';
  sub_array_1[1]          = 'i';
  sub_array_1[2]          = 't';
  sub_array_1[3]          = 't';
  sub_array_1[4]          = 'y';

  EXPECT_EQ(sub_array_1, "kitty");
  EXPECT_EQ(bc, "hello kitty");

  ByteArray sub_array_2 = sub_array_1.SubArray(2, 2);
  EXPECT_EQ(sub_array_2, "tt");
}

TEST(reference_byte_array_gtest, testing_that_ConstByteArray_r_value_moved_if_unique)
{
  char const *   base = "hello world";
  ConstByteArray expected_to_be_moved{base};
  EXPECT_EQ(expected_to_be_moved.UseCount(), 1);

  ByteArray copy{std::move(expected_to_be_moved)};
  EXPECT_EQ(expected_to_be_moved.UseCount(), 0);  // NOLINT(bugprone-use-after-move)

  copy[0] = 'k';
  copy[1] = 'i';
  copy[2] = 't';
  copy[3] = 't';
  copy[4] = 'y';
  EXPECT_EQ(copy, "kitty world");
}

TEST(reference_byte_array_gtest, testing_that_ConstByteArray_r_value_not_moved_if_not_unique)
{
  char const *   base = "hello world";
  ConstByteArray expected_to_remain_unchanged{base};
  ConstByteArray expected_to_remain_unchanged_2{expected_to_remain_unchanged};
  EXPECT_EQ(expected_to_remain_unchanged.UseCount(), 2);
  EXPECT_EQ(expected_to_remain_unchanged_2.UseCount(), expected_to_remain_unchanged.UseCount());

  ByteArray copy{std::move(expected_to_remain_unchanged_2)};
  EXPECT_EQ(expected_to_remain_unchanged_2.UseCount(), 2);  // NOLINT(bugprone-use-after-move)

  copy[0] = 'k';
  copy[1] = 'i';
  copy[2] = 't';
  copy[3] = 't';
  copy[4] = 'y';

  EXPECT_EQ(copy, "kitty world");
  EXPECT_EQ(expected_to_remain_unchanged, base);
  EXPECT_EQ(expected_to_remain_unchanged_2, base);  // NOLINT(bugprone-use-after-move)
}

TEST(reference_byte_array_gtest,
     testing_that_instantiation_of_ByteArray_is_done_by_referencee_and_NOT_by_value)
{
  char const *base = "hello world";
  ByteArray   str(base);
  ByteArray   cba{base};
  ByteArray   copy{cba};  //* This shall make copy by reference
  copy[0] = 'k';
  copy[1] = 'i';
  copy[2] = 't';
  copy[3] = 't';
  copy[4] = 'y';

  char const *const expected = "kitty world";
  EXPECT_EQ(copy, expected);
  EXPECT_EQ(cba, expected);
}

TEST(reference_byte_array_gtest,
     testing_assignment_from_ByteArray_is_done_by_referencee_and_NOT_by_value)
{
  char const *base = "hello world";
  ByteArray   str(base);
  ByteArray   cba{base};
  ByteArray   copy;
  copy    = cba;  //* This shall make copy by reference
  copy[0] = 'k';
  copy[1] = 'i';
  copy[2] = 't';
  copy[3] = 't';
  copy[4] = 'y';

  char const *const expected = "kitty world";
  EXPECT_EQ(copy, expected);
  EXPECT_EQ(cba, expected);
}

TEST(reference_byte_array_gtest, testing_that_instantiation_from_ConstByteArray_is_done_by_value)
{
  char const *   base = "hello world";
  ByteArray      str(base);
  ConstByteArray cba{base};
  ByteArray      copy{cba};  //* This shall make deep copy
  copy[0] = 'k';
  copy[1] = 'i';
  copy[2] = 't';
  copy[3] = 't';
  copy[4] = 'y';

  EXPECT_EQ(cba, base);
  EXPECT_EQ(copy, "kitty world");
}

TEST(reference_byte_array_gtest, testing_that_assignemnt_from_ConstByteArray_is_done_by_value)
{
  char const *   base = "hello world";
  ByteArray      str(base);
  ConstByteArray cba{base};
  ByteArray      copy;
  copy    = cba;  //* This shall make deep copy
  copy[0] = 'k';
  copy[1] = 'i';
  copy[2] = 't';
  copy[3] = 't';
  copy[4] = 'y';

  EXPECT_EQ(cba, base);
  EXPECT_EQ(copy, "kitty world");
}

TEST(reference_byte_array_gtest, testing_that_referencing_vanishes_after_copying)
{
  char const *base = "hello kitty";
  ByteArray   str(base);
  ByteArray   copy = str.Copy().SubArray(6, str.size() - 6);
  copy[0]          = 'Z';
  copy[1]          = 'i';
  copy[2]          = 'p';
  copy[3]          = 'p';
  copy[4]          = 'y';

  EXPECT_EQ(copy, "Zippy");
  EXPECT_EQ(str, "hello kitty");
}
TEST(reference_byte_array_gtest, basic_concat_operations)
{
  char const *base = "hello kitty";
  ByteArray   str(base);
  EXPECT_EQ((str + " kat"), "hello kitty kat");
  EXPECT_EQ(("Big " + str), "Big hello kitty");
}
TEST(reference_byte_array_gtest, basic_append_operations)
{
  ByteArray      v0("hello");
  ConstByteArray v1("pretty");
  ByteArray      v2("kitty");

  ByteArray array;
  array.Append(v0, " ", v1, " ", v2, ' ', std::uint8_t(':'), ")");

  EXPECT_EQ(array, "hello pretty kitty :)");
  EXPECT_EQ(v0, "hello");
  EXPECT_EQ(v1, "pretty");
  EXPECT_EQ(v2, "kitty");

  v0[0] = 'c';
  v0[1] = 'i';
  v0[2] = 'a';
  v0[3] = 'o';
  v0[4] = ' ';
  EXPECT_EQ(v0, "ciao ");

  v2[0] = 'c';
  v2[1] = 'a';
  v2[2] = 't';
  v2[3] = ' ';
  v2[4] = ' ';
  EXPECT_EQ(v2, "cat  ");

  EXPECT_EQ(array, "hello pretty kitty :)");
}

TEST(reference_byte_array_gtest, size_of_loaded_C_strings)
{
  EXPECT_EQ(ByteArray("any carnal pleas").size(), 16);
  EXPECT_EQ(ByteArray("any carnal pleasu").size(), 17);
  EXPECT_EQ(ByteArray("any carnal pleasur").size(), 18);
  EXPECT_EQ(ByteArray("any carnal pleasure").size(), 19);
  EXPECT_EQ(ByteArray("any carnal pleasure.").size(), 20);
}
