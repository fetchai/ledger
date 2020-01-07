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

#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

namespace {

class PairTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(PairTests, assign_u32_string_test)
{
  static char const *TEXT = R"(
    function main()
      var data = Pair<UInt32, String>();

      data.first(2u32);
      data.second("TEST");

      print(data.first());
      print('-');
      print(data.second());

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "2-TEST");
}

TEST_F(PairTests, assign_string_u32_test)
{
  static char const *TEXT = R"(
    function main()
      var data = Pair<String, UInt32>();

      data.first("TEST");
      data.second(2u32);

      print(data.first());
      print('-');
      print(data.second());

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "TEST-2");
}

TEST_F(PairTests, assign_bool_int8_test)
{
  static char const *TEXT = R"(
    function main()
      var data = Pair<Bool, Int8>();

      data.first(true);
      data.second(-4i8);

      print(data.first());
      print(',');
      print(data.second());

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "true,-4");
}

TEST_F(PairTests, assign_uint8_int16_test)
{
  static char const *TEXT = R"(
    function main()
      var data = Pair<UInt8, Int16>();

      data.first(255u8);
      data.second(-12345i16);

      print(data.first());
      print(',');
      print(data.second());

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "255,-12345");
}

TEST_F(PairTests, assign_uint32_int32_test)
{
  static char const *TEXT = R"(
    function main()
      var data = Pair<UInt32, Int32>();

      data.first(1234567u32);
      data.second(-1234567i32);

      print(data.first());
      print(',');
      print(data.second());

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "1234567,-1234567");
}

TEST_F(PairTests, assign_uint64_int64_test)
{
  static char const *TEXT = R"(
    function main()
      var data = Pair<UInt64, Int64>();

      data.first(1234567890u32);
      data.second(-1234567890i32);

      print(data.first());
      print(',');
      print(data.second());

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "1234567890,-1234567890");
}

TEST_F(PairTests, assign_fixed32_fixed64_test)
{
  static char const *TEXT = R"(
    function main()
      var data = Pair<Fixed32, Fixed64>();

      data.first(-234.567fp32);
      data.second(-123.456fp64);

      print(data.first());
      print(',');
      print(data.second());

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "-234.5669,-123.455999999");
}

TEST_F(PairTests, assign_fixed128_uint256_test)
{
  static char const *TEXT = R"(
    function main()
      var data = Pair<Fixed128, String>();

      data.first(-234.567fp128);
      data.second("Hello");

      print(data.first());
      print(',');
      print(data.second());

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "-234.5669999999999999999,Hello");
}

}  // namespace
