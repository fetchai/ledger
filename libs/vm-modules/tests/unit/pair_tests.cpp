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

}  // namespace
