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

TEST_F(PrintTests, print_works_for_arrays_of_8_bit_integers_with_multiple_elements)
{
  static char const *TEXT = R"(
    function main()
      var i = Array<Int8>(3);
      i[0] = -1i8; i[1] = 0i8; i[2] = 1i8;
      print(i);

      var u = Array<UInt8>(3);
      u[0] = 0u8; u[1] = 1u8; u[2] = 2u8;
      print(u);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[-1, 0, 1][0, 1, 2]");
}

TEST_F(PrintTests, print_works_for_arrays_of_8_bit_integers_with_one_element)
{
  static char const *TEXT = R"(
    function main()
      var i = Array<Int8>(1);
      i[0] = -42i8;
      print(i);

      var u = Array<UInt8>(1);
      u[0] = 42u8;
      print(u);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[-42][42]");
}

TEST_F(PrintTests, print_works_for_empty_arrays_of_8_bit_integers)
{
  static char const *TEXT = R"(
    function main()
      var i = Array<Int8>(0);
      print(i);

      var u = Array<UInt8>(0);
      print(u);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[][]");
}

TEST_F(PrintTests, print_works_for_arrays_of_16_bit_integers_with_multiple_elements)
{
  static char const *TEXT = R"(
    function main()
      var i = Array<Int16>(3);
      i[0] = -1i16; i[1] = 0i16; i[2] = 1i16;
      print(i);

      var u = Array<UInt16>(3);
      u[0] = 0u16; u[1] = 1u16; u[2] = 2u16;
      print(u);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[-1, 0, 1][0, 1, 2]");
}

TEST_F(PrintTests, print_works_for_arrays_of_16_bit_integers_with_one_element)
{
  static char const *TEXT = R"(
    function main()
      var i = Array<Int16>(1);
      i[0] = -42i16;
      print(i);

      var u = Array<UInt16>(1);
      u[0] = 42u16;
      print(u);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[-42][42]");
}

TEST_F(PrintTests, print_works_for_empty_arrays_of_16_bit_integers)
{
  static char const *TEXT = R"(
    function main()
      var i = Array<Int16>(0);
      print(i);

      var u = Array<UInt16>(0);
      print(u);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[][]");
}

TEST_F(PrintTests, print_works_for_arrays_of_32_bit_integers_with_multiple_elements)
{
  static char const *TEXT = R"(
    function main()
      var i = Array<Int32>(3);
      i[0] = -1i32; i[1] = 0i32; i[2] = 1i32;
      print(i);

      var u = Array<UInt32>(3);
      u[0] = 0u32; u[1] = 1u32; u[2] = 2u32;
      print(u);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[-1, 0, 1][0, 1, 2]");
}

TEST_F(PrintTests, print_works_for_arrays_of_32_bit_integers_with_one_element)
{
  static char const *TEXT = R"(
    function main()
      var i = Array<Int32>(1);
      i[0] = -42i32;
      print(i);

      var u = Array<UInt32>(1);
      u[0] = 42u32;
      print(u);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[-42][42]");
}

TEST_F(PrintTests, print_works_for_empty_arrays_of_32_bit_integers)
{
  static char const *TEXT = R"(
    function main()
      var i = Array<Int32>(0);
      print(i);

      var u = Array<UInt32>(0);
      print(u);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[][]");
}

TEST_F(PrintTests, print_works_for_arrays_of_64_bit_integers_with_multiple_elements)
{
  static char const *TEXT = R"(
    function main()
      var i = Array<Int64>(3);
      i[0] = -1i64; i[1] = 0i64; i[2] = 1i64;
      print(i);

      var u = Array<UInt64>(3);
      u[0] = 0u64; u[1] = 1u64; u[2] = 2u64;
      print(u);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[-1, 0, 1][0, 1, 2]");
}

TEST_F(PrintTests, print_works_for_arrays_of_64_bit_integers_with_one_element)
{
  static char const *TEXT = R"(
    function main()
      var i = Array<Int64>(1);
      i[0] = -42i64;
      print(i);

      var u = Array<UInt64>(1);
      u[0] = 42u64;
      print(u);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[-42][42]");
}

TEST_F(PrintTests, print_works_for_empty_arrays_of_64_bit_integers)
{
  static char const *TEXT = R"(
    function main()
      var i = Array<Int64>(0);
      print(i);

      var u = Array<UInt64>(0);
      print(u);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[][]");
}

}  // namespace
