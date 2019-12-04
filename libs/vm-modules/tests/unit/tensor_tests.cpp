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
#include "vm_modules/math/ndarray.hpp"
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

TEST_F(VMTensorTests, ndarray_1_dim_creation)
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

TEST_F(VMTensorTests, ndarray_2_dim_creation)
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

TEST_F(VMTensorTests, ndarray_3_dim_creation)
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

TEST_F(VMTensorTests, ndarray_4_dim_creation)
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

TEST_F(VMTensorTests, ndarray_squeeze)
{
  static char const *SOURCE = R"(
    function main() : NDArray<Fixed64>
      var tensor_shape = Array<UInt64>(5);
      tensor_shape[0] = 4u64;
      tensor_shape[1] = 2u64;
      tensor_shape[2] = 1u64;
      tensor_shape[3] = 2u64;
      tensor_shape[4] = 4u64;
      var x = NDArray<Fixed64>(tensor_shape);
      var squeezed_x = x.squeeze();
      return squeezed_x;
    endfunction
  )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run(&res));

  auto const tensor =
      res.Get<Ptr<typename fetch::vm_modules::math::NDArray<fetch::fixed_point::fp64_t>>>();
  auto const constructed_shape = tensor->shape();

  // Expected shape of a squeezed [4,2,1,2,4] is [4,2,2,4]
  fetch::math::Tensor<fetch::fixed_point::fp64_t> expected({4, 2, /*1,*/ 2, 4});

  EXPECT_TRUE(constructed_shape == expected.shape());
}

TEST_F(VMTensorTests, ndarray_unsqueeze)
{
  static char const *SOURCE = R"(
    function main() : NDArray<Fixed64>
      var tensor_shape = Array<UInt64>(4);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 3u64;
      tensor_shape[2] = 4u64;
      tensor_shape[3] = 5u64;
      var x = NDArray<Fixed64>(tensor_shape);
      var unsqueezed_x = x.unsqueeze();
      return unsqueezed_x;
    endfunction
  )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run(&res));

  auto const tensor =
      res.Get<Ptr<typename fetch::vm_modules::math::NDArray<fetch::fixed_point::fp64_t>>>();
  auto const constructed_shape = tensor->shape();

  // Expected shape of a unsqueezed [2,3,4,5] is [2,3,4,5,1]
  fetch::math::Tensor<fetch::fixed_point::fp64_t> expected({2, 3, 4, 5, 1});

  EXPECT_TRUE(constructed_shape == expected.shape());
}

// TODO(VH): 1,3,4,5,6 dims also.
TEST_F(VMTensorTests, ndarray_2_dim_fill)
{
  static char const *SOURCE = R"(
      function main()
        var tensor_shape = Array<UInt64>(2);
        tensor_shape[0] = 10u64;
        tensor_shape[1] = 10u64;

        var a = NDArray<Float32>(tensor_shape);
        a.fill(7.0f);
        assert(a.at(0,0) == 7.0f);

        var b = NDArray<Float64>(tensor_shape);
        b.fill(7.0);
        assert(b.at(1,0) == 7.0);

        var c = NDArray<Fixed32>(tensor_shape);
        c.fill(7.0fp32);
        assert(c.at(0,1) == 7.0fp32);

        var d = NDArray<Fixed64>(tensor_shape);
        d.fill(7.0fp64);
        assert(d.at(1,1) == 7.0fp64);
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run());
}

// Disabled until implementation completed
TEST_F(VMTensorTests, DISABLED_ndarray_state_test)
{
  static char const *tensor_serialiase_src = R"(
    function main()
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 10u64;
      var x = NDArray<Fixed64>(tensor_shape);
      x.fill(7.0fp64);
      var state = State<NDArray<Fixed64>>("tensor");
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

// Disabled because of runtime crash when .at() or [] is called
TEST_F(VMTensorTests, DISABLED_ndarray_2_dim_inplace_subtraction)
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
       assert(float32_2[0,0] == float32_zeros[0,0]);
       printLn(toString(float32_2.at(1,0)));
     endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run());
}

}  // namespace
