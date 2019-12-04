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

#include "gmock/gmock.h"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_test_toolkit.hpp"

#include <regex>
#include <sstream>

using namespace fetch::vm;

namespace {

using DataType = fetch::vm_modules::math::DataType;
using ::testing::Between;

class VMTensorTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};  // namespace

TEST_F(VMTensorTests, ndarray_creation_1_dim)
{
  static char const *SOURCE = R"(
     function main()
        var tensor_shape = Array<UInt64>(1);
        tensor_shape[0] = 10u64;
        var float32 = NDArray<Float32>(tensor_shape);
        var float64 = NDArray<Float64>(tensor_shape);
        var fixed32 = NDArray<Fixed32>(tensor_shape);
        var fixed64 = NDArray<Fixed64>(tensor_shape);
     endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(VMTensorTests, ndarray_creation_2_dim)
{
  static char const *SOURCE = R"(
     function main()
        var tensor_shape = Array<UInt64>(2);
        tensor_shape[0] = 10u64;
        tensor_shape[1] = 10u64;
        var float32 = NDArray<Float32>(tensor_shape);
        var float64 = NDArray<Float64>(tensor_shape);
        var fixed32 = NDArray<Fixed32>(tensor_shape);
        var fixed64 = NDArray<Fixed64>(tensor_shape);
     endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(VMTensorTests, ndarray_creation_3_dim)
{
  static char const *SOURCE = R"(
     function main()
        var tensor_shape = Array<UInt64>(3);
        tensor_shape[0] = 10u64;
        tensor_shape[1] = 10u64;
        tensor_shape[2] = 10u64;
        var float32 = NDArray<Float32>(tensor_shape);
        var float64 = NDArray<Float64>(tensor_shape);
        var fixed32 = NDArray<Fixed32>(tensor_shape);
        var fixed64 = NDArray<Fixed64>(tensor_shape);
     endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(VMTensorTests, ndarray_creation_4_dim)
{
  static char const *SOURCE = R"(
     function main()
        var tensor_shape = Array<UInt64>(4);
        tensor_shape[0] = 10u64;
        tensor_shape[1] = 10u64;
        tensor_shape[2] = 10u64;
        tensor_shape[3] = 10u64;
        var float32 = NDArray<Float32>(tensor_shape);
        var float64 = NDArray<Float64>(tensor_shape);
        var fixed32 = NDArray<Fixed32>(tensor_shape);
        var fixed64 = NDArray<Fixed64>(tensor_shape);
     endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run());
}

// Disabled until implementation completed
TEST_F(VMTensorTests, DISABLED_tensor_squeeze_test)
{
  static char const *tensor_serialiase_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(3);
      tensor_shape[0] = 4u64;
      tensor_shape[1] = 1u64;
      tensor_shape[2] = 4u64;
      var x = NDArray(tensor_shape);
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

// Disabled until implementation completed
TEST_F(VMTensorTests, DISABLED_tensor_state_test)
{
  static char const *tensor_serialiase_src = R"(
    function main()
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 10u64;
      var x = NDArray(tensor_shape);
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

// Disabled until implementation completed
TEST_F(VMTensorTests, DISABLED_tensor_set_and_at_one_test)
{
  static char const *tensor_serialiase_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(1);
      tensor_shape[0] = 2u64;

      var x = NDArray(tensor_shape);
      var y = NDArray(tensor_shape);
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

// Disabled until implementation completed
TEST_F(VMTensorTests, DISABLED_tensor_set_and_at_two_test)
{
  static char const *tensor_serialiase_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 2u64;

      var x = NDArray(tensor_shape);
      var y = NDArray(tensor_shape);
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

// Disabled until implementation completed
TEST_F(VMTensorTests, DISABLED_tensor_set_and_at_three_test)
{
  static char const *tensor_serialiase_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(3);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 2u64;
      tensor_shape[2] = 2u64;

      var x = NDArray(tensor_shape);
      var y = NDArray(tensor_shape);
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

// Disabled until implementation completed
TEST_F(VMTensorTests, DISABLED_tensor_set_and_at_four_test)
{
  static char const *tensor_serialiase_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(4);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 2u64;
      tensor_shape[2] = 2u64;
      tensor_shape[3] = 2u64;

      var x = NDArray(tensor_shape);
      var y = NDArray(tensor_shape);
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

// Disabled until implementation completed
TEST_F(VMTensorTests, DISABLED_tensor_set_from_string)
{
  static char const *tensor_from_string_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(3);
      tensor_shape[0] = 4u64;
      tensor_shape[1] = 1u64;
      tensor_shape[2] = 1u64;

      var x = NDArray(tensor_shape);
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

// Disabled until implementation completed
TEST_F(VMTensorTests, ndarray_inplace_subtraction)
{
  static char const *SOURCE = R"(
     function main()
       var tensor_shape = Array<UInt64>(2);
       tensor_shape[0] = 2u64;
       tensor_shape[1] = 2u64;
       var float32_1 = NDArray<Float32>(tensor_shape);
       float32_1[0,0] = 111.0f;
       float32_1[1,0] = 222.0f;
       float32_1[0,1] = 333.0f;
       float32_1[1,1] = 444.0f;
       var float32_2 = NDArray<Float32>(tensor_shape);
       float32_2[0,0] = 111.0f;
       float32_2[1,0] = 222.0f;
       float32_2[0,1] = 333.0f;
       float32_2[1,1] = 444.0f;
       var float32_zeros = NDArray<Float32>(tensor_shape);
       float32_2 -= float32_1;
       var a : Float32 = float32_2.at(1,0);
       //assert(float32_2[0,0] == float32_zeros[0,0]);
       printLn(toString(float32_2.at(1,0)));
     endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run());
}

}  // namespace
