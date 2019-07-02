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
#include "math/standard_functions/pow.hpp"

#include "vm_modules/math/math.hpp"
#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

using namespace fetch::vm;

namespace {

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
  ASSERT_TRUE(toolkit.Run());

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
  ASSERT_TRUE(toolkit.Run());

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
  ASSERT_TRUE(toolkit.Run());

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
  ASSERT_TRUE(toolkit.Run());

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<float_t>();

  float gt  = 3.5;
  float exp = 2.0;
  gt        = fetch::math::Pow(gt, exp);

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
  ASSERT_TRUE(toolkit.Run());

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<float_t>();

  float gt = 3.5;
  gt       = fetch::math::Sqrt(gt);

  ASSERT_EQ(result, gt);
}

TEST_F(MathTests, tensor_state_test)
{
  static char const *tensor_serialiase_src = R"(
    function main()
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 10u64;
      var x = Tensor(tensor_shape);
      x.Fill(7.0f);
      var state = State<Tensor>("tensor");
      state.set(x);
    endfunction
  )";

  std::string const state_name{"tensor"};
  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));

  ASSERT_TRUE(toolkit.Compile(tensor_serialiase_src));
  ASSERT_TRUE(toolkit.Run());

  static char const *tensor_deserialiase_src = R"(
    function main() : Tensor
      var state = State<Tensor>("tensor");
      return state.get();
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Exists(state_name));
  EXPECT_CALL(toolkit.observer(), Read(state_name, _, _));

  ASSERT_TRUE(toolkit.Compile(tensor_deserialiase_src));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                 tensor = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  fetch::math::Tensor<float> gt({2, 10});
  gt.Fill(7.0);

  EXPECT_TRUE(gt.AllClose(tensor->GetTensor()));
}
}  // namespace
