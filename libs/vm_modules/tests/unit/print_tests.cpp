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

// TODO(WK)
TEST_F(PrintTests, DISABLED_print_works_for_8_bit_integers)
{
  static char const *TEXT = R"(
    function main()
//      var u = 42u8;
//      print(u);
//      print(', ');

//      var pos_i = 42i8;
//      print(pos_i);
//      print(', ');

//      var neg_i = -42i8;
//      print(neg_i);
   endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "42, 42, -42");
}

// TODO(WK)
TEST_F(PrintTests, DISABLED_print_works_for_16_bit_integers)
{
  static char const *TEXT = R"(
    function main()
//      var u = 42u16;
//      print(u);
//      print(', ');

//      var pos_i = 42i16;
//      print(pos_i);
//      print(', ');

//      var neg_i = -42i16;
//      print(neg_i);
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

}  // namespace
