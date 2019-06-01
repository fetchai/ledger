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

class PrintTests : public ::testing::Test
{
public:
  VmTestToolkit toolkit;
};

TEST_F(PrintTests, print_works_for_8_bit_integers)
{
  static char const *TEXT = R"(
    function main()
      var u = 42u8;
      print(u);
      print(', ');

      var pos_i = 42i8;
      print(pos_i);
      print(', ');

      var neg_i = -42i8;
      print(neg_i);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "42, 42, -42");
}

TEST_F(PrintTests, print_works_for_16_bit_integers)
{
  static char const *TEXT = R"(
    function main()
      var u = 42u16;
      print(u);
      print(', ');

      var pos_i = 42i16;
      print(pos_i);
      print(', ');

      var neg_i = -42i16;
      print(neg_i);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "42, 42, -42");
}

TEST_F(PrintTests, print_works_for_32_bit_integers)
{
  static char const *TEXT = R"(
    function main()
      var u = 42u32;
      print(u);
      print(', ');

      var pos_i = 42i32;
      print(pos_i);
      print(', ');

      var neg_i = -42i32;
      print(neg_i);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "42, 42, -42");
}

TEST_F(PrintTests, print_works_for_64_bit_integers)
{
  static char const *TEXT = R"(
    function main()
      var u = 42u64;
      print(u);
      print(', ');

      var pos_i = 42i64;
      print(pos_i);
      print(', ');

      var neg_i = -42i64;
      print(neg_i);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "42, 42, -42");
}

// TODO(WK)
TEST_F(PrintTests, DISABLED_print_works_for_arrays_of_8_bit_integers_with_multiple_elements)
{
  static char const *TEXT = R"(
    function main()
      var arr_i8 = Array<Int8>(3);
      arr_i8[0] = -1i8; arr_i8[1] = 0i8; arr_i8[2] = 1i8;
      print(arr_i8);

      var arr_u8 = Array<UInt8>(3);
      arr_u8[0] = 0u8; arr_u8[1] = 1u8; arr_u8[2] = 2u8;
      print(arr_u8);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[-1, 0, 1][0, 1, 2]");
}

// TODO(WK)
TEST_F(PrintTests, DISABLED_print_works_for_arrays_of_16_bit_integers_with_multiple_elements)
{
  static char const *TEXT = R"(
    function main()
      var arr_i16 = Array<Int16>(3);
      arr_i16[0] = -1i16; arr_i16[1] = 0i16; arr_i16[2] = 1i16;
      print(arr_i16);

      var arr_u16 = Array<UInt16>(3);
      arr_u16[0] = 0u16; arr_u16[1] = 1u16; arr_u16[2] = 2u16;
      print(arr_u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[-1, 0, 1][0, 1, 2]");
}

TEST_F(PrintTests, print_works_for_arrays_of_32_bit_integers_with_multiple_elements)
{
  static char const *TEXT = R"(
    function main()
      var arr_i32 = Array<Int32>(3);
      arr_i32[0] = -1i32; arr_i32[1] = 0i32; arr_i32[2] = 1i32;
      print(arr_i32);

      var arr_u32 = Array<UInt32>(3);
      arr_u32[0] = 0u32; arr_u32[1] = 1u32; arr_u32[2] = 2u32;
      print(arr_u32);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[-1, 0, 1][0, 1, 2]");
}

TEST_F(PrintTests, print_works_for_arrays_of_64_bit_integers_with_multiple_elements)
{
  static char const *TEXT = R"(
    function main()
      var arr_i64 = Array<Int64>(3);
      arr_i64[0] = -1i64; arr_i64[1] = 0i64; arr_i64[2] = 1i64;
      print(arr_i64);

      var arr_u64 = Array<UInt64>(3);
      arr_u64[0] = 0u64; arr_u64[1] = 1u64; arr_u64[2] = 2u64;
      print(arr_u64);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[-1, 0, 1][0, 1, 2]");
}

}  // namespace
