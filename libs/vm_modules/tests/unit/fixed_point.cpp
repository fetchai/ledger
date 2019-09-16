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

#include "math/base_types.hpp"
#include "math/trigonometry.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <sstream>
#include <string>

using namespace fetch::fixed_point;

namespace {

class FixedPointTest : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

bool RunTest(VmTestToolkit &toolkit, std::stringstream &stdout, char const *TEXT, double gt)
{
  EXPECT_TRUE(toolkit.Compile(TEXT));
  EXPECT_TRUE(toolkit.Run());
  EXPECT_NEAR(std::stod(stdout.str()), gt, double(fetch::math::function_tolerance<fp32_t>()));
  return true;
}

TEST_F(FixedPointTest, create_fixed_point)
{
  char const *TEXT = R"(
    function main()
      print(1.0fp32);
    endfunction
  )";
  double      gt   = static_cast<double>(fp32_t(1));
  EXPECT_TRUE(RunTest(toolkit, stdout, TEXT, gt));
}

TEST_F(FixedPointTest, addition_fixed_point)
{
  char const *TEXT = R"(
    function main()
      var a = 2.0fp32;
      var b = 3.0fp32;
      a += b;
      print(a);
    endfunction
  )";
  double      gt   = static_cast<double>(fp32_t(5));
  EXPECT_TRUE(RunTest(toolkit, stdout, TEXT, gt));
}

TEST_F(FixedPointTest, subtraction_fixed_point)
{
  char const *TEXT = R"(
    function main()
      var a = 3.0fp32;
      var b = 2.0fp32;
      a -= b;
      print(a);
    endfunction
  )";
  double      gt   = static_cast<double>(fp32_t(1));
  EXPECT_TRUE(RunTest(toolkit, stdout, TEXT, gt));
}

TEST_F(FixedPointTest, multiplication_fixed_point)
{
  char const *TEXT = R"(
    function main()
      var a = 3.0fp32;
      var b = 2.0fp32;
      a *= b;
      print(a);
    endfunction
  )";
  double      gt   = static_cast<double>(fp32_t(6));
  EXPECT_TRUE(RunTest(toolkit, stdout, TEXT, gt));
}

TEST_F(FixedPointTest, divide_fixed_point)
{
  char const *TEXT = R"(
    function main()
      var a = 3.0fp32;
      var b = 2.0fp32;
      a /= b;
      print(a);
    endfunction
  )";
  double      gt   = static_cast<double>(fp32_t(1.5));
  EXPECT_TRUE(RunTest(toolkit, stdout, TEXT, gt));
}

TEST_F(FixedPointTest, array_32_fixed_point)
{
  char const *TEXT = R"(
    function main()
      var myArray = Array<Fixed32>(5);

      for (i in 0:4)
        myArray[i] = toFixed32(i);
      endfor
      print(myArray[3]);
    endfunction
  )";
  double      gt   = static_cast<double>(fp32_t(3));
  EXPECT_TRUE(RunTest(toolkit, stdout, TEXT, gt));
}

TEST_F(FixedPointTest, map_32_fixed_point)
{
  char const *TEXT = R"(
    function main()
      var mymap = Map<Fixed32, Fixed32>();
      mymap[0fp32] = 1fp32;
      print(mymap[0fp32]);
    endfunction
  )";
  double      gt   = static_cast<double>(fp32_t(1));
  EXPECT_TRUE(RunTest(toolkit, stdout, TEXT, gt));
}

TEST_F(FixedPointTest, sin_pi_32_fixed_point)
{
  char const *TEXT = R"(
    function main()
      var pi = 3.1415fp32;
      print(sin(pi));
    endfunction
  )";
  double      gt   = static_cast<double>(fetch::math::Sin(fp32_t::CONST_PI));
  EXPECT_TRUE(RunTest(toolkit, stdout, TEXT, gt));
}

TEST_F(FixedPointTest, cos_pi_32_fixed_point)
{
  char const *TEXT = R"(
    function main()
      var pi = 3.1415fp32;
      print(cos(pi));
    endfunction
  )";
  double      gt   = static_cast<double>(fetch::math::Cos(fp32_t(fp32_t::CONST_PI)));
  EXPECT_TRUE(RunTest(toolkit, stdout, TEXT, gt));
}

TEST_F(FixedPointTest, exp_32_fixed_point)
{
  char const *TEXT = R"(
    function main()
      var val = 1fp32;
      print(exp(val));
    endfunction
  )";
  double      gt   = static_cast<double>(fp32_t(fp32_t::CONST_E));
  EXPECT_TRUE(RunTest(toolkit, stdout, TEXT, gt));
}

TEST_F(FixedPointTest, pow_32_fixed_point)
{
  char const *TEXT = R"(
    function main()
      var val = 2fp32;
      print(pow(val, val));
    endfunction
  )";
  double      gt   = static_cast<double>(fp32_t(4));
  EXPECT_TRUE(RunTest(toolkit, stdout, TEXT, gt));
}

}  // namespace
