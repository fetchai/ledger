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

#include "vm_modules/math/math.hpp"
#include "math/standard_functions/abs.hpp"
#include "vm_test_toolkit.hpp"

namespace {

class MathTests : public ::testing::Test
{
public:
  VmTestToolkit toolkit;
};

TEST_F(MathTests, abs_test)
{
  static char const *TEXT = R"(
    function main()
      print(Abs(-1));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), std::to_string(fetch::math::Abs(-1)));
}

}  // namespace
