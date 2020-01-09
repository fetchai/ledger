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

#include "math/standard_functions/abs.hpp"
#include "math/standard_functions/exp.hpp"
#include "math/standard_functions/log.hpp"
#include "math/standard_functions/pow.hpp"
#include "math/standard_functions/sqrt.hpp"
#include "vm/fixed.hpp"
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

TEST_F(MathTests, exp32_test)
{
  static char const *TEXT = R"(
   function main() : Fixed32
     return exp(3.5fp32);
   endfunction
 )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<fetch::fixed_point::fp32_t>();

  fetch::fixed_point::fp32_t gt{"3.5"};
  fetch::math::Exp(gt, gt);

  ASSERT_EQ(result, gt);
}

TEST_F(MathTests, exp64_test)
{
  static char const *TEXT = R"(
   function main() : Fixed64
     return exp(3.5fp64);
   endfunction
 )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<fetch::fixed_point::fp64_t>();

  fetch::fixed_point::fp64_t gt{"3.5"};
  fetch::math::Exp(gt, gt);

  ASSERT_EQ(result, gt);
}

TEST_F(MathTests, exp128_test)
{
  static char const *TEXT = R"(
   function main() : Fixed128
     return exp(3.5fp128);
   endfunction
 )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<Ptr<Fixed128>>();

  fetch::fixed_point::fp128_t gt{"3.5"};
  fetch::math::Exp(gt, gt);

  ASSERT_EQ(result->data_, gt);
}

TEST_F(MathTests, log32_test)
{
  static char const *TEXT = R"(
   function main() : Fixed32
     return log(3.5fp32);
   endfunction
 )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<fetch::fixed_point::fp32_t>();

  fetch::fixed_point::fp32_t gt{"3.5"};
  fetch::math::Log(gt, gt);

  ASSERT_EQ(result, gt);
}

TEST_F(MathTests, log64_test)
{
  static char const *TEXT = R"(
   function main() : Fixed64
     return log(3.5fp64);
   endfunction
 )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<fetch::fixed_point::fp64_t>();

  fetch::fixed_point::fp64_t gt{"3.5"};
  fetch::math::Log(gt, gt);

  ASSERT_EQ(result, gt);
}

TEST_F(MathTests, pow32_test)
{
  static char const *TEXT = R"(
   function main() : Fixed32
     return pow(3.5fp32, 2.0fp32);
   endfunction
 )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<fetch::fixed_point::fp32_t>();

  fetch::fixed_point::fp32_t gt{"3.5"};
  fetch::fixed_point::fp32_t p{"2.0"};
  fetch::math::Pow(gt, p, gt);

  ASSERT_EQ(result, gt);
}

TEST_F(MathTests, pow64_test)
{
  static char const *TEXT = R"(
   function main() : Fixed64
     return pow(3.5fp64, 2.0fp64);
   endfunction
 )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<fetch::fixed_point::fp64_t>();

  fetch::fixed_point::fp64_t gt{"3.5"};
  fetch::fixed_point::fp64_t p{"2.0"};
  fetch::math::Pow(gt, p, gt);

  ASSERT_EQ(result, gt);
}

TEST_F(MathTests, sqrt32_test)
{
  static char const *TEXT = R"(
   function main() : Fixed32
     return sqrt(3.5fp32);
   endfunction
 )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<fetch::fixed_point::fp32_t>();

  fetch::fixed_point::fp32_t gt{"3.5"};
  fetch::math::Sqrt(gt, gt);

  ASSERT_EQ(result, gt);
}

TEST_F(MathTests, sqrt64_test)
{
  static char const *TEXT = R"(
   function main() : Fixed64
     return sqrt(3.5fp64);
   endfunction
 )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<fetch::fixed_point::fp64_t>();

  fetch::fixed_point::fp64_t gt{"3.5"};
  fetch::math::Sqrt(gt, gt);

  ASSERT_EQ(result, gt);
}

TEST_F(MathTests, rand32_test)
{
  static char const *TEXT = R"(
   function main() : Fixed32
     return rand(1.0fp32, 1000fp32);
   endfunction
 )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<fetch::fixed_point::fp32_t>();

  fetch::fixed_point::fp32_t a{"1.0"};
  fetch::fixed_point::fp32_t b{"1000.0"};

  ASSERT_TRUE(result >= a);
  ASSERT_TRUE(result <= b);
}

TEST_F(MathTests, rand64_test)
{
  static char const *TEXT = R"(
   function main() : Fixed64
     return rand(1.0fp64, 1000fp64);
   endfunction
 )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<fetch::fixed_point::fp64_t>();

  fetch::fixed_point::fp64_t a{"1.0"};
  fetch::fixed_point::fp64_t b{"1000.0"};

  ASSERT_TRUE(result >= a);
  ASSERT_TRUE(result <= b);
}

TEST_F(MathTests, rand128_test)
{
  static char const *TEXT = R"(
   function main() : Fixed128
     return rand(1.0fp128, 1000fp128);
   endfunction
 )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<Ptr<Fixed128>>();

  fetch::fixed_point::fp128_t a{"1.0"};
  fetch::fixed_point::fp128_t b{"1000.0"};

  ASSERT_TRUE(result->data_ >= a);
  ASSERT_TRUE(result->data_ <= b);
}

}  // namespace
