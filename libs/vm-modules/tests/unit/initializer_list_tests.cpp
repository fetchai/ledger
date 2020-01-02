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

#include <algorithm>
#include <utility>

namespace {

class InitializerListTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(InitializerListTests, used_in_var_statements)
{
  static char const *testCase = R"(
    function main()
      var b: Array<Int32> = {4, 5, 6, 7};
      var d: Array<Int32> = {};
      print(b);
      print(d);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(testCase)) << stdout.str();
  ASSERT_TRUE(toolkit.Run()) << stdout.str();

  ASSERT_EQ(stdout.str(), "[4, 5, 6, 7][]");
}

TEST_F(InitializerListTests, used_in_invoke_expressions)
{
  static char const *TEXT = R"(
    function secondary(a: Array<Int32>)
      print(a);
    endfunction
    function main()
      secondary({0, 1, 314});
      secondary({});
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT)) << stdout.str();
  ASSERT_TRUE(toolkit.Run()) << stdout.str();

  ASSERT_EQ(stdout.str(), "[0, 1, 314][]");
}

TEST_F(InitializerListTests, empty_init_list_fails_type_inference)
{
  auto const TEXT = R"(
    function main()
      var x = {};
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(InitializerListTests, non_empty_init_list_fails_type_inference)
{
  auto const TEXT = R"(
    function main()
      var x = {1, 2, 3};
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(InitializerListTests, always_homogeneous)
{
  static char const *testCase = R"(
    function main()
      var b: Array<Int32> = {4, 5.6};
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(testCase));
}

}  // namespace
