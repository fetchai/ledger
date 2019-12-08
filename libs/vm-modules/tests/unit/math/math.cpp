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

#include "math/standard_functions/abs.hpp"
#include "math/standard_functions/exp.hpp"
#include "math/standard_functions/log.hpp"
#include "math/standard_functions/pow.hpp"
#include "math/standard_functions/sqrt.hpp"
#include "vm_modules/math/math.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <sstream>

using namespace fetch::vm;

namespace {

using DataType = fetch::vm_modules::math::DataType;

class MathTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(MathTests, abs_test)
{
  static char const *TEXT = R"(
    function main() : Int32
      return abs(-1);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<int>();

  int gt = -1;
  fetch::math::Abs(gt, gt);

  ASSERT_EQ(result, gt);
}

TEST_F(MathTests, exp_test)
{
  static char const *TEXT = R"(
    function main() : Float32
      return exp(3.5f);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<float_t>();

  float gt = 3.5;
  fetch::math::Exp(gt, gt);

  ASSERT_EQ(result, gt);
}

TEST_F(MathTests, log_test)
{
  static char const *TEXT = R"(
    function main() : Float32
      return log(3.5f);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<float_t>();

  float gt = 3.5;
  fetch::math::Log(gt, gt);

  ASSERT_EQ(result, gt);
}

TEST_F(MathTests, pow_test)
{
  static char const *TEXT = R"(
    function main() : Float32
      return pow(3.5f, 2.0f);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<float_t>();

  float const gt = fetch::math::Pow(3.5f, 2.0f);

  ASSERT_EQ(result, gt);
}

TEST_F(MathTests, sqrt_test)
{
  static char const *TEXT = R"(
    function main() : Float32
      return sqrt(3.5f);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<float_t>();

  float const gt = fetch::math::Sqrt(3.5f);

  ASSERT_EQ(result, gt);
}

}  // namespace
