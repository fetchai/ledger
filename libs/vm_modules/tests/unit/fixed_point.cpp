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

#include "vm_modules/math/fixed_point_wrapper.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm_test_toolkit.hpp"

namespace {

class FixedPointTest : public ::testing::Test
{
public:
  VmTestToolkit toolkit;
};


TEST_F(FixedPointTest, create_fixed_point)
{
  auto m = toolkit.module();
  fetch::vm_modules::CreateFixedPoint(m);

  static char const *TEXT = R"(
    function main()
      print(FixedPoint(1.0));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  double gt = static_cast<double>(fetch::fixed_point::fp32_t(1));
  EXPECT_EQ(toolkit.stdout(), std::to_string(gt));
}

}  // namespace
