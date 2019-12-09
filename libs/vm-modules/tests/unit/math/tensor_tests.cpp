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
#include "vm_modules/math/math.hpp"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <sstream>

using namespace fetch::vm;

namespace {

using ::testing::Between;

using DataType = fetch::vm_modules::math::DataType;

class MathTensorTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

/// GETTER AND SETTER TESTS ///

TEST_F(MathTensorTests, tensor_1_dim_fixed64_fill)
{
  static char const *FILL_1_DIM_SRC = R"(
            function main()
              var tensor_shape = Array<UInt64>(1);
              tensor_shape[0] = 10u64;
              var d = Tensor(tensor_shape);
              d.fill(toFixed64(123456.0));
              assert(d.at(1u64) == toFixed64(123456.0));
            endfunction
          )";
  ASSERT_TRUE(toolkit.Compile(FILL_1_DIM_SRC));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_2_dim_fixed64_fill)
{
  static char const *FILL_2_DIM_SRC = R"(
            function main()
              var tensor_shape = Array<UInt64>(2);
              tensor_shape[0] = 10u64;
              tensor_shape[1] = 10u64;
              var d = Tensor(tensor_shape);
              d.fill(toFixed64(123456.0));
              assert(d.at(1u64,1u64) == toFixed64(123456.0));
            endfunction
          )";

  ASSERT_TRUE(toolkit.Compile(FILL_2_DIM_SRC));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_3_dim_fixed64_fill)
{
  static char const *FILL_3_DIM_SRC = R"(
            function main()
              var tensor_shape = Array<UInt64>(3);
              tensor_shape[0] = 10u64;
              tensor_shape[1] = 10u64;
              tensor_shape[2] = 10u64;
              var d = Tensor(tensor_shape);
              d.fill(toFixed64(123456.0));
              assert(d.at(1u64,1u64,1u64) == toFixed64(123456.0));
            endfunction
          )";

  ASSERT_TRUE(toolkit.Compile(FILL_3_DIM_SRC));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_4_dim_fixed64_fill)
{
  static char const *FILL_4_DIM_SRC = R"(
            function main()
              var tensor_shape = Array<UInt64>(4);
              tensor_shape[0] = 10u64;
              tensor_shape[1] = 10u64;
              tensor_shape[2] = 10u64;
              tensor_shape[3] = 10u64;
              var d = Tensor(tensor_shape);
              d.fill(toFixed64(123456.0));
              assert(d.at(1u64,1u64,1u64,1u64) == toFixed64(123456.0));
            endfunction
          )";
  ASSERT_TRUE(toolkit.Compile(FILL_4_DIM_SRC));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_at_on_invalid_index)
{
  static char const *SRC = R"(
    function main()
      var tensor_shape = Array<UInt64>(1);
      tensor_shape[0] = 2u64;

      var x = Tensor(tensor_shape);

      printLn(toString(x.at(3u64)));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_at_invalid_index_count_too_many)
{
  static char const *SRC = R"(
    function main()
      var tensor_shape = Array<UInt64>(1);
      tensor_shape[0] = 2u64;

      var x = Tensor(tensor_shape);

      printLn(toString(x.at(0u64, 0u64)));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_at_invalid_index_count_too_few)
{
  static char const *SRC = R"(
    function main()
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 2u64;

      var x = Tensor(tensor_shape);

      printLn(toString(x.at(0u64)));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_set_on_invalid_index)
{
  static char const *SRC = R"(
    function main()
      var tensor_shape = Array<UInt64>(1);
      tensor_shape[0] = 2u64;

      var x = Tensor(tensor_shape);

      x.setAt(3u64, 1.0fp64);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_set_and_at_one_test)
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

TEST_F(MathTensorTests, tensor_set_and_at_two_test)
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

TEST_F(MathTensorTests, tensor_set_and_at_three_test)
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

TEST_F(MathTensorTests, tensor_set_and_at_four_test)
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

TEST_F(MathTensorTests, tensor_set_from_string)
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

/// MATRIX OPERATION TESTS ///

TEST_F(MathTensorTests, tensor_min_test)
{
  fetch::math::Tensor<DataType> tensor = fetch::math::Tensor<DataType>::FromString(
      "0.5, 7.1, 9.1; 6.2, 7.1, 4.; -99.1, 14328.1, 10.0;");
  fetch::vm_modules::math::VMTensor vm_tensor(&toolkit.vm(), 0, tensor);

  DataType result = vm_tensor.Min();
  DataType gt{-99.1};

  EXPECT_TRUE(result == gt);
}

TEST_F(MathTensorTests, tensor_min_etch_test)
{
  static char const *tensor_min_src = R"(
    function main() : Fixed64
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 3u64;
      tensor_shape[1] = 3u64;
      var x = Tensor(tensor_shape);
      x.fill(7.0fp64);
      x.setAt(0u64, 1u64, -7.0fp64);
      x.setAt(1u64, 1u64, 23.1fp64);
      var ret = x.min();
      return ret;
    endfunction
  )";

  std::string const state_name{"tensor"};

  ASSERT_TRUE(toolkit.Compile(tensor_min_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const min_val = res.Get<DataType>();
  DataType   gt{-7.0};

  EXPECT_TRUE(gt == min_val);
}

TEST_F(MathTensorTests, tensor_max_test)
{
  fetch::math::Tensor<DataType> tensor = fetch::math::Tensor<DataType>::FromString(
      "0.5, 7.1, 9.1; 6.2, 7.1, 4.; -99.1, 14328.1, 10.0;");
  fetch::vm_modules::math::VMTensor vm_tensor(&toolkit.vm(), 0, tensor);

  DataType result = vm_tensor.Max();
  DataType gt{14328.1};

  EXPECT_TRUE(result == gt);
}

TEST_F(MathTensorTests, tensor_max_etch_test)
{
  static char const *tensor_max_src = R"(
    function main() : Fixed64
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 3u64;
      tensor_shape[1] = 3u64;
      var x = Tensor(tensor_shape);
      x.fill(7.0fp64);
      x.setAt(0u64, 1u64, -7.0fp64);
      x.setAt(1u64, 1u64, 23.1fp64);
      var ret = x.max();
      return ret;
    endfunction
  )";

  std::string const state_name{"tensor"};

  ASSERT_TRUE(toolkit.Compile(tensor_max_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const max_val = res.Get<DataType>();
  DataType   gt{23.1};

  std::cout << "gt: " << gt << std::endl;
  std::cout << "max_val: " << max_val << std::endl;

  EXPECT_TRUE(gt == max_val);
}

TEST_F(MathTensorTests, tensor_sum_test)
{
  fetch::math::Tensor<DataType> tensor = fetch::math::Tensor<DataType>::FromString(
      "0.5, 7.1, 9.1; 6.2, 7.1, 4.; -99.1, 14328.1, 10.0;");
  fetch::vm_modules::math::VMTensor vm_tensor(&toolkit.vm(), 0, tensor);

  DataType result = vm_tensor.Sum();
  DataType gt{14273.0};

  EXPECT_TRUE(fetch::math::Abs(gt - result) < DataType::TOLERANCE);
}

TEST_F(MathTensorTests, tensor_sum_etch_test)
{
  static char const *tensor_sum_src = R"(
    function main() : Fixed64
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 3u64;
      tensor_shape[1] = 3u64;
      var x = Tensor(tensor_shape);
      x.fill(7.0fp64);
      x.setAt(0u64, 1u64, -7.0fp64);
      x.setAt(1u64, 1u64, 23.1fp64);
      var ret = x.sum();
      return ret;
    endfunction
  )";

  std::string const state_name{"tensor"};

  ASSERT_TRUE(toolkit.Compile(tensor_sum_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const sum_val = res.Get<DataType>();
  DataType   gt{65.1};

  EXPECT_TRUE(fetch::math::Abs(gt - sum_val) < DataType::TOLERANCE);
}

TEST_F(MathTensorTests, tensor_squeeze_test)
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

TEST_F(MathTensorTests, tensor_invalid_squeeze_test)
{
  static char const *SOURCE = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(3);
      tensor_shape[0] = 4u64;
      tensor_shape[1] = 4u64;
      tensor_shape[2] = 4u64;
      var x = Tensor(tensor_shape);
      var squeezed_x = x.squeeze();
      return squeezed_x;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_unsqueeze_test)
{
  static char const *SOURCE = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(4);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 3u64;
      tensor_shape[2] = 4u64;
      tensor_shape[3] = 5u64;
      var x = Tensor(tensor_shape);
      var unsqueezed_x = x.unsqueeze();
      return unsqueezed_x;
    endfunction
  )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run(&res));

  auto const tensor            = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto const constructed_shape = tensor->shape();

  // Expected shape of a unsqueezed [2,3,4,5] is [2,3,4,5,1]
  fetch::math::Tensor<fetch::fixed_point::fp64_t> expected({2, 3, 4, 5, 1});

  EXPECT_TRUE(constructed_shape == expected.shape());
}

/// SERIALISATION TESTS ///

TEST_F(MathTensorTests, tensor_state_test)
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

TEST_F(MathTensorTests, tensor_invalid_from_string)
{
  static char const *SOURCE = R"(
      function main()
        var tensor_shape = Array<UInt64>(3);
        tensor_shape[0] = 4u64;
        tensor_shape[1] = 1u64;
        tensor_shape[2] = 1u64;

        var x = Tensor(tensor_shape);
        x.fill(2.0fp64);

        var string_vals = "INVALID_STRING";
        x.fromString(string_vals);
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_FALSE(toolkit.Run());
}

}  // namespace
