//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <iostream>

#include "core/byte_array/byte_array.hpp"
using namespace fetch::byte_array;

#include "testing/unittest.hpp"

int main()
{
  SCENARIO("Class members of ByteArray")
  {
    char const *base    = "hello world";
    std::string basecpp = base;
    ByteArray   str(base);

    EXPECT(str == base);
    EXPECT(str == basecpp);

    SECTION("ensuring that subbyte_arrays come out correctly")
    {
      EXPECT(str.SubArray(0, 5) == "hello");
      EXPECT(str.SubArray(0, 5) == basecpp.substr(0, 5));

      EXPECT(str.SubArray(6, str.size() - 6) == "world");
      EXPECT(str.SubArray(6, str.size() - 6) == basecpp.substr(6, basecpp.size() - 6));
    };

    SECTION("testing that referencing persists after subbyte_arraying ")
    {
      ByteArray copy = str.SubArray(6, str.size() - 6);
      copy[0]        = 'k';
      copy[1]        = 'i';
      copy[2]        = 't';
      copy[3]        = 't';
      copy[4]        = 'y';

      EXPECT(str == "hello kitty");
    };

    SECTION("testing sub-array of sub-array")
    {
      ByteArray bc{base};
      ByteArray sub_array_1 = bc.SubArray(bc.size() - 5, 5);
      sub_array_1[0]        = 'k';
      sub_array_1[1]        = 'i';
      sub_array_1[2]        = 't';
      sub_array_1[3]        = 't';
      sub_array_1[4]        = 'y';

      EXPECT(sub_array_1 == "kitty");
      EXPECT(bc == "hello kitty");

      ByteArray sub_array_2 = sub_array_1.SubArray(2, 2);
      EXPECT(sub_array_2 == "tt");
    };

    SECTION(
        "testing that ConstByteArray r-value.is moved if it is *unique* (when there are *no* other "
        "ConstByteArray instances sharing the same underlying data.")
    {
      ConstByteArray expected_to_be_moved{base};
      EXPECT(expected_to_be_moved.UseCount() == 1);

      ByteArray copy{std::move(expected_to_be_moved)};
      EXPECT(expected_to_be_moved.UseCount() == 0);  // NOLINT(bugprone-use-after-move)

      copy[0] = 'k';
      copy[1] = 'i';
      copy[2] = 't';
      copy[3] = 't';
      copy[4] = 'y';
      EXPECT(copy == "kitty world");
    };

    SECTION(
        "testing that ConstByteArray r-value.is *not* moved if it is *not* unique (when it shares "
        "underlying data with other(s) ConstByteArray instances.")
    {
      ConstByteArray expected_to_remain_unchanged{base};
      ConstByteArray expected_to_remain_unchanged_2{expected_to_remain_unchanged};
      EXPECT(expected_to_remain_unchanged.UseCount() == 2);
      EXPECT(expected_to_remain_unchanged_2.UseCount() == expected_to_remain_unchanged.UseCount());

      ByteArray copy{std::move(expected_to_remain_unchanged_2)};
      EXPECT(expected_to_remain_unchanged_2.UseCount() == 2);  // NOLINT(bugprone-use-after-move)

      copy[0] = 'k';
      copy[1] = 'i';
      copy[2] = 't';
      copy[3] = 't';
      copy[4] = 'y';

      EXPECT(copy == "kitty world");
      EXPECT(expected_to_remain_unchanged == base);
      EXPECT(expected_to_remain_unchanged_2 == base);  // NOLINT(bugprone-use-after-move)
    };

    SECTION(
        "testing that instantiation from ByteArray (copy-construct) is done by referencee and "
        "*NOT* by value(deep copy).")
    {
      ByteArray cba{base};
      ByteArray copy{cba};  //* This shall make copy by reference
      copy[0] = 'k';
      copy[1] = 'i';
      copy[2] = 't';
      copy[3] = 't';
      copy[4] = 'y';

      char const *const expected = "kitty world";
      EXPECT(copy == expected);
      EXPECT(cba == expected);
    };

    SECTION(
        "testing that assignment from ByteArray is done by referencee and *NOT* by value(deep "
        "copy).")
    {
      ByteArray cba{base};
      ByteArray copy;
      copy    = cba;  //* This shall make copy by reference
      copy[0] = 'k';
      copy[1] = 'i';
      copy[2] = 't';
      copy[3] = 't';
      copy[4] = 'y';

      char const *const expected = "kitty world";
      EXPECT(copy == expected);
      EXPECT(cba == expected);
    };

    SECTION(
        "testing that instantiation from ConstByteArray is done by value(deep copy) and *not* by "
        "reference.")
    {
      ConstByteArray cba{base};
      ByteArray      copy{cba};  //* This shall make deep copy
      copy[0] = 'k';
      copy[1] = 'i';
      copy[2] = 't';
      copy[3] = 't';
      copy[4] = 'y';

      EXPECT(cba == base);
      EXPECT(copy == "kitty world");
    };

    SECTION(
        "testing that assignemnt from ConstByteArray is done by value(deep copy) and *not* by "
        "reference.")
    {
      ConstByteArray cba{base};
      ByteArray      copy;
      copy    = cba;  //* This shall make deep copy
      copy[0] = 'k';
      copy[1] = 'i';
      copy[2] = 't';
      copy[3] = 't';
      copy[4] = 'y';

      EXPECT(cba == base);
      EXPECT(copy == "kitty world");
    };

    // Even though the previous section copied that byte_array object, the
    // underlying
    // data is still referenced.
    EXPECT(str == "hello kitty");

    SECTION("testing that referencing vanishes after copying ")
    {
      ByteArray copy = str.Copy().SubArray(6, str.size() - 6);
      copy[0]        = 'Z';
      copy[1]        = 'i';
      copy[2]        = 'p';
      copy[3]        = 'p';
      copy[4]        = 'y';

      EXPECT(copy == "Zippy");
      EXPECT(str == "hello kitty");
    };

    SECTION("basic concat operations")
    {
      EXPECT((str + " kat") == "hello kitty kat");
      EXPECT(("Big " + str) == "Big hello kitty");
    };

    SECTION("size of loaded C strings")
    {
      EXPECT(ByteArray("any carnal pleas").size() == 16);
      EXPECT(ByteArray("any carnal pleasu").size() == 17);
      EXPECT(ByteArray("any carnal pleasur").size() == 18);
      EXPECT(ByteArray("any carnal pleasure").size() == 19);
      EXPECT(ByteArray("any carnal pleasure.").size() == 20);
    };
  };

  return 0;
}
