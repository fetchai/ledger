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

#include "gtest/gtest.h"

#include <memory>

namespace {

using fetch::vm::VM;

class VmChargeTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(VmChargeTests, NoLimitTest)
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
  ASSERT_EQ(0, toolkit.vm().GetChargeLimit());
  ASSERT_TRUE(toolkit.Run());

  EXPECT_EQ(18, toolkit.vm().GetChargeTotal());
}

TEST_F(VmChargeTests, ExecutionFailsWhenLimitReached)
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
  toolkit.vm().SetChargeLimit(10);
  ASSERT_EQ(10, toolkit.vm().GetChargeLimit());
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(VmChargeTests, DISABLED_BindWithChargeEstimate)
{
  auto estimator = [](VM *, uint32_t x, uint32_t y) -> uint64_t {
    auto const base = x * y;
    return base * base;
  };
  FETCH_UNUSED(estimator);

  toolkit.module()->CreateFreeFunction("expensive", [](VM *, uint32_t, uint32_t) {
    return true;
  });

  static char const *TEXT = R"(
      function main()
        var check = expensive(3, 3);
        assert(check == true);
      endfunction
    )";


  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

} // namespace