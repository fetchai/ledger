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
#include "vm_modules/math/math.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <sstream>

using namespace fetch::vm;

namespace {

using ::testing::Between;

using DataType = fetch::vm_modules::math::DataType;

class MathTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};

  // tensor charge estimation enforcement
  ChargeAmount tensor_setup_charge_min{16};
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

TEST_F(MathTests, tensor_squeeze_test)
{
  static char const *tensor_serialiase_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(3);
      tensor_shape[0] = 4u64;
      tensor_shape[1] = 1u64;
      tensor_shape[2] = 4u64;
      var x = Tensor(tensor_shape);
      var squeezed_x = x.squeeze();
      return squeezed_x;
    endfunction
  )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(tensor_serialiase_src));
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  fetch::math::Tensor<DataType> gt({4, 4});

  EXPECT_TRUE(tensor->GetTensor().shape() == gt.shape());
}

TEST_F(MathTests, tensor_state_test)
{
  static char const *tensor_serialiase_src = R"(
    function main()
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 10u64;
      var x = Tensor(tensor_shape);
      x.fill(7.0fp64);
      var state = State<Tensor>("tensor");
      state.set(x);
    endfunction
  )";

  std::string const state_name{"tensor"};

  ASSERT_TRUE(toolkit.Compile(tensor_serialiase_src));

  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));
  ASSERT_TRUE(toolkit.Run());

  static char const *tensor_deserialiase_src = R"(
    function main() : Tensor
      var state = State<Tensor>("tensor");
      return state.get();
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_deserialiase_src));

  Variant res;
  EXPECT_CALL(toolkit.observer(), Exists(state_name));
  EXPECT_CALL(toolkit.observer(), Read(state_name, _, _)).Times(Between(1, 2));
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  fetch::math::Tensor<DataType> gt({2, 10});
  gt.Fill(static_cast<DataType>(7.0));

  EXPECT_TRUE(gt.AllClose(tensor->GetTensor()));
}

TEST_F(MathTests, tensor_set_and_at_one_test)
{
  static char const *tensor_serialiase_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(1);
      tensor_shape[0] = 2u64;

      var x = Tensor(tensor_shape);
      var y = Tensor(tensor_shape);
      x.fill(2.0fp64);

      y.setAt(0u64,x.at(0u64));
      y.setAt(1u64,x.at(1u64));

     return y;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_serialiase_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  fetch::math::Tensor<DataType> gt({2});
  gt.Fill(static_cast<DataType>(2.0));

  EXPECT_TRUE(gt.AllClose(tensor->GetTensor()));
}

TEST_F(MathTests, tensor_set_and_at_two_test)
{
  static char const *tensor_serialiase_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 2u64;

      var x = Tensor(tensor_shape);
      var y = Tensor(tensor_shape);
      x.fill(2.0fp64);

      y.setAt(0u64,0u64,x.at(0u64,0u64));
      y.setAt(0u64,1u64,x.at(0u64,1u64));
      y.setAt(1u64,0u64,x.at(1u64,0u64));
      y.setAt(1u64,1u64,x.at(1u64,1u64));

     return y;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_serialiase_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  fetch::math::Tensor<DataType> gt({2, 2});
  gt.Fill(static_cast<DataType>(2.0));

  EXPECT_TRUE(gt.AllClose(tensor->GetTensor()));
}

TEST_F(MathTests, tensor_set_and_at_three_test)
{
  static char const *tensor_serialiase_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(3);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 2u64;
      tensor_shape[2] = 2u64;

      var x = Tensor(tensor_shape);
      var y = Tensor(tensor_shape);
      x.fill(2.0fp64);

      y.setAt(0u64,0u64,0u64,x.at(0u64,0u64,0u64));
      y.setAt(0u64,1u64,0u64,x.at(0u64,1u64,0u64));
      y.setAt(1u64,0u64,0u64,x.at(1u64,0u64,0u64));
      y.setAt(1u64,1u64,0u64,x.at(1u64,1u64,0u64));
      y.setAt(0u64,0u64,1u64,x.at(0u64,0u64,1u64));
      y.setAt(0u64,1u64,1u64,x.at(0u64,1u64,1u64));
      y.setAt(1u64,0u64,1u64,x.at(1u64,0u64,1u64));
      y.setAt(1u64,1u64,1u64,x.at(1u64,1u64,1u64));

     return y;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_serialiase_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  fetch::math::Tensor<DataType> gt({2, 2, 2});
  gt.Fill(static_cast<DataType>(2.0));

  EXPECT_TRUE(gt.AllClose(tensor->GetTensor()));
}

TEST_F(MathTests, tensor_set_and_at_four_test)
{
  static char const *tensor_serialiase_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(4);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 2u64;
      tensor_shape[2] = 2u64;
      tensor_shape[3] = 2u64;

      var x = Tensor(tensor_shape);
      var y = Tensor(tensor_shape);
      x.fill(2.0fp64);

      y.setAt(0u64,0u64,0u64,0u64,x.at(0u64,0u64,0u64,0u64));
      y.setAt(0u64,1u64,0u64,0u64,x.at(0u64,1u64,0u64,0u64));
      y.setAt(1u64,0u64,0u64,0u64,x.at(1u64,0u64,0u64,0u64));
      y.setAt(1u64,1u64,0u64,0u64,x.at(1u64,1u64,0u64,0u64));
      y.setAt(0u64,0u64,1u64,0u64,x.at(0u64,0u64,1u64,0u64));
      y.setAt(0u64,1u64,1u64,0u64,x.at(0u64,1u64,1u64,0u64));
      y.setAt(1u64,0u64,1u64,0u64,x.at(1u64,0u64,1u64,0u64));
      y.setAt(1u64,1u64,1u64,0u64,x.at(1u64,1u64,1u64,0u64));
      y.setAt(0u64,0u64,0u64,1u64,x.at(0u64,0u64,0u64,1u64));
      y.setAt(0u64,1u64,0u64,1u64,x.at(0u64,1u64,0u64,1u64));
      y.setAt(1u64,0u64,0u64,1u64,x.at(1u64,0u64,0u64,1u64));
      y.setAt(1u64,1u64,0u64,1u64,x.at(1u64,1u64,0u64,1u64));
      y.setAt(0u64,0u64,1u64,1u64,x.at(0u64,0u64,1u64,1u64));
      y.setAt(0u64,1u64,1u64,1u64,x.at(0u64,1u64,1u64,1u64));
      y.setAt(1u64,0u64,1u64,1u64,x.at(1u64,0u64,1u64,1u64));
      y.setAt(1u64,1u64,1u64,1u64,x.at(1u64,1u64,1u64,1u64));

     return y;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_serialiase_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  fetch::math::Tensor<DataType> gt({2, 2, 2, 2});
  gt.Fill(static_cast<DataType>(2.0));

  EXPECT_TRUE(gt.AllClose(tensor->GetTensor()));
}

TEST_F(MathTests, tensor_set_from_string)
{
  static char const *tensor_from_string_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(3);
      tensor_shape[0] = 4u64;
      tensor_shape[1] = 1u64;
      tensor_shape[2] = 1u64;

      var x = Tensor(tensor_shape);
      x.fill(2.0fp64);

      var string_vals = "1.0, 1.0, 1.0, 1.0";
      x.fromString(string_vals);

      return x;

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_from_string_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  fetch::math::Tensor<DataType> gt({4, 1, 1});
  gt.Fill(static_cast<DataType>(1.0));

  EXPECT_TRUE(gt.AllClose(tensor->GetTensor()));
}

TEST_F(MathTests, tensor_setup_charge_estimator_min_charge_succeeds)
{
  static char const *tensor_setup_src = R"(
    function main()
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 5u64;
      tensor_shape[1] = 2u64;
      var x = Tensor(tensor_shape);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_setup_src));
  ASSERT_TRUE(toolkit.Run(nullptr, tensor_setup_charge_min)) << stdout.str();
}

TEST_F(MathTests, tensor_setup_charge_estimator_min_charge_fails)
{
  static char const *tensor_setup_src = R"(
    function main()
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 5u64;
      tensor_shape[1] = 2u64;
      var x = Tensor(tensor_shape);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_setup_src));
  ASSERT_FALSE(toolkit.Run(nullptr, tensor_setup_charge_min - 1)) << stdout.str();
}

TEST_F(MathTests, tensor_fill_estimator_succeeds)
{
  size_t       tensor_size = 10;
  ChargeAmount charge_max  = tensor_setup_charge_min + 2 * fetch::vm::CHARGE_UNIT * tensor_size;

  static char const *tensor_to_string_src = R"(
    function main()
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 5u64;
      tensor_shape[1] = 2u64;
      var x = Tensor(tensor_shape);
      x.fill(2.0fp64);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_to_string_src));
  ASSERT_TRUE(toolkit.Run(nullptr, charge_max)) << stdout.str();
}

TEST_F(MathTests, tensor_fill_estimator_fails)
{
  size_t       tensor_size = 10;
  ChargeAmount charge_max  = tensor_setup_charge_min + 2 * fetch::vm::CHARGE_UNIT * tensor_size;

  static char const *tensor_to_string_src = R"(
    function main()
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 5u64;
      tensor_shape[1] = 5u64;
      var x = Tensor(tensor_shape);
      x.fill(2.0fp64);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_to_string_src));
  ASSERT_FALSE(toolkit.Run(nullptr, charge_max));
}

TEST_F(MathTests, tensor_to_string_estimator_succeeds)
{
  size_t       tensor_size = 10;
  ChargeAmount charge_max  = tensor_setup_charge_min + 2 * fetch::vm::CHARGE_UNIT * tensor_size;

  static char const *tensor_to_string_src = R"(
    function main()
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 5u64;
      tensor_shape[1] = 2u64;
      var x = Tensor(tensor_shape);
      x.toString();
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_to_string_src));
  ASSERT_TRUE(toolkit.Run(nullptr, charge_max));
}

TEST_F(MathTests, tensor_to_string_estimator_fails)
{
  size_t       tensor_size = 10;
  ChargeAmount charge_max  = tensor_setup_charge_min + 2 * fetch::vm::CHARGE_UNIT * tensor_size;

  static char const *tensor_to_string_src = R"(
    function main()
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 5u64;
      tensor_shape[1] = 2u64;
      var x = Tensor(tensor_shape);
      x.toString();
      x.toString();
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_to_string_src));
  ASSERT_FALSE(toolkit.Run(nullptr, charge_max));
}

}  // namespace
