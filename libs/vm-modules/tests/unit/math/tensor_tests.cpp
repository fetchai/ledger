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

#include "gmock/gmock.h"
#include "math/standard_functions/abs.hpp"
#include "vm/array.hpp"
#include "vm_modules/math/math.hpp"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_test_toolkit.hpp"

#include <sstream>

using namespace fetch::vm;

namespace {

using ::testing::Between;

using DataType = fetch::vm_modules::math::DataType;
using SizeType = fetch::vm_modules::math::SizeType;

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
              assert(d.at(1u64) != 123456.123456fp64);
              d.fill(123456.123456fp64);
              assert(d.at(1u64) == 123456.123456fp64);
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
              assert(d.at(1u64,1u64) != 123456.123456fp64);
              d.fill(123456.123456fp64);
              assert(d.at(1u64,1u64) == 123456.123456fp64);
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
              assert(d.at(1u64,1u64,1u64) != 123456.123456fp64);
              d.fill(123456.123456fp64);
              assert(d.at(1u64,1u64,1u64) == 123456.123456fp64);
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
              assert(d.at(1u64,1u64,1u64,1u64) != 123456.123456fp64);
              d.fill(123456.123456fp64);
              assert(d.at(1u64,1u64,1u64,1u64) == 123456.123456fp64);
            endfunction
          )";
  ASSERT_TRUE(toolkit.Compile(FILL_4_DIM_SRC));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_construction_from_string_1_fixed64)
{
  static char const *STR_CONSTRUCT_SRC = R"(
            function main()
              var d = Tensor("1.0, 2.0");
              assert(d.at(0u64,0u64) == 1.0fp64);
              assert(d.at(0u64,1u64) == 2.0fp64);
            endfunction
          )";
  ASSERT_TRUE(toolkit.Compile(STR_CONSTRUCT_SRC));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_construction_from_string_2_fixed64)
{
  static char const *STR_CONSTRUCT_SRC = R"(
            function main()
              var d = Tensor("1.0, 2.0; 3.0, 4.0");
              assert(d.at(0u64,0u64) == 1.0fp64);
              assert(d.at(0u64,1u64) == 2.0fp64);
              assert(d.at(1u64,0u64) == 3.0fp64);
              assert(d.at(1u64,1u64) == 4.0fp64);

            endfunction
          )";
  ASSERT_TRUE(toolkit.Compile(STR_CONSTRUCT_SRC));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_construction_from_malformed_string_1_fixed64)
{
  static char const *STR_CONSTRUCT_SRC = R"(
            function main()
              var d = Tensor("1.0.0, 2.0");
            endfunction
          )";
  ASSERT_TRUE(toolkit.Compile(STR_CONSTRUCT_SRC));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_construction_from_malformed_string_2_fixed64)
{
  static char const *STR_CONSTRUCT_SRC = R"(
            function main()
              var d = Tensor("1.0, 2.0; 3.0");
            endfunction
          )";
  ASSERT_TRUE(toolkit.Compile(STR_CONSTRUCT_SRC));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_construction_from_malformed_string_3_fixed64)
{
  static char const *STR_CONSTRUCT_SRC = R"(
            function main()
              var d = Tensor("");
            endfunction
          )";
  ASSERT_TRUE(toolkit.Compile(STR_CONSTRUCT_SRC));
  ASSERT_FALSE(toolkit.Run());
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
  gt.Fill(fetch::math::Type<DataType>("2.0"));

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
  gt.Fill(fetch::math::Type<DataType>("2.0"));

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
  gt.Fill(fetch::math::Type<DataType>("2.0"));

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
  gt.Fill(fetch::math::Type<DataType>("2.0"));

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
  gt.Fill(fetch::math::Type<DataType>("1.0"));

  EXPECT_TRUE(gt.AllClose(tensor->GetTensor()));
}

TEST_F(MathTensorTests, tensor_shape_from_tensor)
{
  static char const *tensor_from_string_src = R"(
    function main() : Array<UInt64>
      var tensor_shape = Array<UInt64>(3);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 3u64;
      tensor_shape[2] = 4u64;
      var x = Tensor(tensor_shape);

      var shape = x.shape();

      return shape;

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_from_string_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto tensor_shape{res.Get<Ptr<IArray>>()};

  std::vector<SizeType>       ret;
  std::vector<SizeType> const gt({2, 3, 4});

  while (tensor_shape->Count() > 0)
  {
    ret.emplace_back(tensor_shape->PopFrontOne().Get<uint64_t>());
  }

  EXPECT_EQ(gt, ret);
}

TEST_F(MathTensorTests, tensor_copy_from_tensor)
{
  static char const *tensor_from_string_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(1);
      tensor_shape[0] = 4u64;

      var x = Tensor(tensor_shape);
      var y = Tensor(tensor_shape);
      x.fill(2.0fp64);
      y = x.copy();
      x.setAt(0u64, 1.0fp64);

      return y;

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_from_string_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  fetch::math::Tensor<DataType> gt({4});
  gt.Fill(DataType{2});

  EXPECT_TRUE(gt.AllClose(tensor->GetTensor()));
}

/// TENSOR ARITHMETIC TESTS ///

TEST_F(MathTensorTests, tensor_equal_etch_test)
{
  static char const *tensor_equal_true_src = R"(
    function main() : Bool
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 3u64;
      tensor_shape[1] = 3u64;
      var x = Tensor(tensor_shape);
      var y = Tensor(tensor_shape);
      x.fill(7.0fp64);
      y.fill(7.0fp64);
      var result : Bool = (x == y);
      return result;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_equal_true_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const result = res.Get<bool>();
  EXPECT_TRUE(result == true);
  // test again for when not equal
  static char const *tensor_equal_false_src = R"(
    function main() : Bool
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 3u64;
      tensor_shape[1] = 3u64;
      var x = Tensor(tensor_shape);
      var y = Tensor(tensor_shape);
      x.fill(7.0fp64);
      y.fill(7.0fp64);
      y.setAt(0u64, 0u64, 1.0fp64);
      var result : Bool = (x == y);
      return result;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_equal_false_src));
  ASSERT_TRUE(toolkit.Run(&res));

  auto const result2 = res.Get<bool>();
  EXPECT_TRUE(result2 == false);
}

TEST_F(MathTensorTests, tensor_not_equal_etch_test)
{

  static char const *tensor_not_equal_true_src = R"(
      function main() : Bool
        var tensor_shape = Array<UInt64>(2);
        tensor_shape[0] = 3u64;
        tensor_shape[1] = 3u64;
        var x = Tensor(tensor_shape);
        var y = Tensor(tensor_shape);
        x.fill(7.0fp64);
        y.fill(7.0fp64);
        var result : Bool = (x != y);
        return result;
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(tensor_not_equal_true_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const result = res.Get<bool>();
  EXPECT_TRUE(result == false);

  // test again for when not equal
  static char const *tensor_not_equal_false_src = R"(
      function main() : Bool
        var tensor_shape = Array<UInt64>(2);
        tensor_shape[0] = 3u64;
        tensor_shape[1] = 3u64;
        var x = Tensor(tensor_shape);
        var y = Tensor(tensor_shape);
        x.fill(7.0fp64);
        y.fill(7.0fp64);
        y.setAt(0u64, 0u64, 1.0fp64);
        var result : Bool = (x != y);
        return result;
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(tensor_not_equal_false_src));
  ASSERT_TRUE(toolkit.Run(&res));

  auto const result2 = res.Get<bool>();
  EXPECT_TRUE(result2 == true);
}

TEST_F(MathTensorTests, tensor_add_test)
{

  static char const *tensor_add_src = R"(
      function main() : Tensor
        var tensor_shape = Array<UInt64>(2);
        tensor_shape[0] = 3u64;
        tensor_shape[1] = 3u64;
        var x = Tensor(tensor_shape);
        var y = Tensor(tensor_shape);
        x.fill(7.0fp64);
        y.fill(7.0fp64);
        var result = x + y;
        return result;
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(tensor_add_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor_ptr = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto                          tensor     = tensor_ptr->GetTensor();
  fetch::math::Tensor<DataType> gt({3, 3});
  gt.Fill(fetch::math::Type<DataType>("14.0"));

  EXPECT_TRUE(gt.AllClose(tensor));
}

TEST_F(MathTensorTests, tensor_subtract_test)
{

  static char const *tensor_add_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 3u64;
      tensor_shape[1] = 3u64;
      var x = Tensor(tensor_shape);
      var y = Tensor(tensor_shape);
      x.fill(7.0fp64);
      y.fill(9.0fp64);
      var result = x - y;
      return result;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_add_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor_ptr = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto                          tensor     = tensor_ptr->GetTensor();
  fetch::math::Tensor<DataType> gt({3, 3});
  gt.Fill(fetch::math::Type<DataType>("-2.0"));

  EXPECT_TRUE(gt.AllClose(tensor));
}

TEST_F(MathTensorTests, tensor_multiply_test)
{

  static char const *tensor_mul_src = R"(
      function main() : Tensor
        var tensor_shape = Array<UInt64>(2);
        tensor_shape[0] = 3u64;
        tensor_shape[1] = 3u64;
        var x = Tensor(tensor_shape);
        var y = Tensor(tensor_shape);
        x.fill(7.0fp64);
        y.fill(7.0fp64);
        var result = x * y;
        return result;
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(tensor_mul_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor_ptr = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto                          tensor     = tensor_ptr->GetTensor();
  fetch::math::Tensor<DataType> gt({3, 3});
  gt.Fill(fetch::math::Type<DataType>("49.0"));

  EXPECT_TRUE(gt.AllClose(tensor));
}

TEST_F(MathTensorTests, tensor_divide_test)
{

  static char const *tensor_div_src = R"(
      function main() : Tensor
        var tensor_shape = Array<UInt64>(2);
        tensor_shape[0] = 3u64;
        tensor_shape[1] = 3u64;
        var x = Tensor(tensor_shape);
        var y = Tensor(tensor_shape);
        x.fill(7.0fp64);
        y.fill(14.0fp64);
        var result = x / y;
        return result;
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(tensor_div_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor_ptr = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto                          tensor     = tensor_ptr->GetTensor();
  fetch::math::Tensor<DataType> gt({3, 3});
  gt.Fill(fetch::math::Type<DataType>("0.5"));

  EXPECT_TRUE(gt.AllClose(tensor));
}

TEST_F(MathTensorTests, tensor_inplace_multiply_test)
{
  static char const *tensor_inplace_mul_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 3u64;
      tensor_shape[1] = 3u64;
      var x = Tensor(tensor_shape);
      var y = Tensor(tensor_shape);
      x.fill(7.0fp64);
      y.fill(7.0fp64);
      x *= y;
      return x;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_inplace_mul_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor_ptr = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto                          tensor     = tensor_ptr->GetTensor();
  fetch::math::Tensor<DataType> gt({3, 3});
  gt.Fill(fetch::math::Type<DataType>("49.0"));

  EXPECT_TRUE(gt.AllClose(tensor));
}

TEST_F(MathTensorTests, tensor_inplace_divide_test)
{
  static char const *tensor_inplace_div_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 3u64;
      tensor_shape[1] = 3u64;
      var x = Tensor(tensor_shape);
      var y = Tensor(tensor_shape);
      x.fill(7.0fp64);
      y.fill(14.0fp64);
      x /= y;
      return x;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_inplace_div_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor_ptr = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto                          tensor     = tensor_ptr->GetTensor();
  fetch::math::Tensor<DataType> gt({3, 3});
  gt.Fill(fetch::math::Type<DataType>("0.5"));

  EXPECT_TRUE(gt.AllClose(tensor));
}

TEST_F(MathTensorTests, tensor_inplace_add_test)
{
  static char const *tensor_add_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 3u64;
      tensor_shape[1] = 3u64;
      var x = Tensor(tensor_shape);
      var y = Tensor(tensor_shape);
      x.fill(7.0fp64);
      y.fill(7.0fp64);
      x += y;
      return x;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_add_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor_ptr = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto                          tensor     = tensor_ptr->GetTensor();
  fetch::math::Tensor<DataType> gt({3, 3});
  gt.Fill(fetch::math::Type<DataType>("14.0"));

  EXPECT_TRUE(gt.AllClose(tensor));
}

TEST_F(MathTensorTests, tensor_inplace_subtract_test)
{
  static char const *tensor_add_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 3u64;
      tensor_shape[1] = 3u64;
      var x = Tensor(tensor_shape);
      var y = Tensor(tensor_shape);
      x.fill(7.0fp64);
      y.fill(9.0fp64);
      x -= y;
      return x;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_add_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor_ptr = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto                          tensor     = tensor_ptr->GetTensor();
  fetch::math::Tensor<DataType> gt({3, 3});
  gt.Fill(fetch::math::Type<DataType>("-2.0"));

  EXPECT_TRUE(gt.AllClose(tensor));
}

TEST_F(MathTensorTests, tensor_negate_etch_test)
{

  static char const *tensor_negate_src = R"(
      function main() : Tensor
        var tensor_shape = Array<UInt64>(2);
        tensor_shape[0] = 3u64;
        tensor_shape[1] = 3u64;
        var x = Tensor(tensor_shape);
        x.fill(7.0fp64);
        x = -x;
        return x;
      endfunction
    )";

  std::string const state_name{"tensor"};

  ASSERT_TRUE(toolkit.Compile(tensor_negate_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const tensor_ptr = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto       tensor     = tensor_ptr->GetTensor();

  fetch::math::Tensor<DataType> gt({3, 3});
  gt.Fill(fetch::math::Type<DataType>("-7.0"));

  EXPECT_TRUE(gt.AllClose(tensor));
}

/// MATRIX OPERATION TESTS ///

TEST_F(MathTensorTests, tensor_min_test)
{
  fetch::math::Tensor<DataType> tensor = fetch::math::Tensor<DataType>::FromString(
      "0.5, 7.1, 9.1; 6.2, 7.1, 4.; -99.1, 14328.1, 10.0;");
  fetch::vm_modules::math::VMTensor vm_tensor(&toolkit.vm(), 0, tensor);

  DataType const result = vm_tensor.Min();
  DataType const expected{"-99.1"};

  EXPECT_EQ(result, expected);
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

  ASSERT_TRUE(toolkit.Compile(tensor_min_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const     min_val = res.Get<DataType>();
  DataType const expected{"-7.0"};

  EXPECT_EQ(expected, min_val);
}

TEST_F(MathTensorTests, tensor_max_test)
{
  fetch::math::Tensor<DataType> tensor = fetch::math::Tensor<DataType>::FromString(
      "0.5, 7.1, 9.1; 6.2, 7.1, 4.; -99.1, 14328.1, 10.0;");
  fetch::vm_modules::math::VMTensor vm_tensor(&toolkit.vm(), 0, tensor);

  DataType const result = vm_tensor.Max();
  DataType const expected{"14328.1"};

  EXPECT_EQ(result, expected);
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

  ASSERT_TRUE(toolkit.Compile(tensor_max_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const     max_val = res.Get<DataType>();
  DataType const expected{"23.1"};

  EXPECT_EQ(expected, max_val);
}

TEST_F(MathTensorTests, tensor_sum_test)
{
  fetch::math::Tensor<DataType> tensor = fetch::math::Tensor<DataType>::FromString(
      "0.5, 7.1, 9.1; 6.2, 7.1, 4.; -99.1, 14328.1, 10.0;");
  fetch::vm_modules::math::VMTensor vm_tensor(&toolkit.vm(), 0, tensor);

  DataType const result = vm_tensor.Sum();
  DataType const expected{"14273.0"};

  EXPECT_LE(fetch::math::Abs(expected - result), DataType::TOLERANCE);
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

  ASSERT_TRUE(toolkit.Compile(tensor_sum_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const     sum_val = res.Get<DataType>();
  DataType const expected{"65.1"};

  EXPECT_LE(fetch::math::Abs(expected - sum_val), DataType::TOLERANCE);
}

TEST_F(MathTensorTests, tensor_transpose_test)
{
  fetch::math::Tensor<DataType> tensor =
      fetch::math::Tensor<DataType>::FromString("1.1, 2.2, 3.3; 4.4, 5.5, 6.6;");
  ASSERT_TRUE(tensor.shape().size() == 2);

  fetch::vm_modules::math::VMTensor vm_tensor(&toolkit.vm(), 0, tensor);

  auto const transposed = vm_tensor.Transpose()->GetTensor();

  DataType const result = transposed.At<fetch::math::SizeType, fetch::math::SizeType>(1, 0);
  DataType const expected{"2.2"};

  EXPECT_EQ(tensor.shape().at(0), transposed.shape().at(1));
  EXPECT_EQ(tensor.shape().at(1), transposed.shape().at(0));

  EXPECT_EQ(expected, result);
}

TEST_F(MathTensorTests, tensor_invalid_shape_transpose_test)
{
  fetch::math::Tensor<DataType> tensor;
  tensor.Reshape({4, 4, 4});
  fetch::vm_modules::math::VMTensor vm_tensor(&toolkit.vm(), 0, tensor);

  EXPECT_THROW(vm_tensor.GetTensor().Transpose(), std::exception);
}

TEST_F(MathTensorTests, tensor_transpose_etch_test)
{
  static char const *SOURCE = R"(
    function main() : Fixed64
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 10u64;
      tensor_shape[1] = 2u64;
      var x = Tensor(tensor_shape);
      x.fill(42.0fp64);
      x.setAt(0u64, 1u64, -1.0fp64);
      var transposed = x.transpose();
      var ret = transposed.at(0u64, 1u64);
      return ret;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const     result = res.Get<DataType>();
  DataType const expected{"42.0"};

  EXPECT_EQ(expected, result);
}

TEST_F(MathTensorTests, tensor_invalid_shape_transpose_etch_test)
{
  static char const *SOURCE = R"(
    function main()
      var tensor_shape = Array<UInt64>(3);
      tensor_shape[0] = 10u64;
      tensor_shape[1] = 2u64;
      tensor_shape[2] = 2u64;
      var x = Tensor(tensor_shape);
      var transposed = x.transpose();
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_reshape_to_invalid_shape_etch_test)
{
  static char const *SRC = R"(
      function main()
        var tensor_shape = Array<UInt64>(2);
        tensor_shape[0] = 2u64;
        tensor_shape[1] = 2u64;

        var x = Tensor(tensor_shape);

        var new_shape = Array<UInt64>(2);
        new_shape[0] = 0u64;
        new_shape[1] = 2u64;

        x.reshape(new_shape);
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_reshape_to_incompatible_shape_etch_test)
{
  static char const *SRC = R"(
    function main()
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 2u64;

      var x = Tensor(tensor_shape);

      var new_shape = Array<UInt64>(2);
      new_shape[0] = 3u64;
      new_shape[1] = 2u64;

      x.reshape(new_shape);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(MathTensorTests, tensor_reshape_to_compatible_shape_test)
{
  using namespace fetch::vm;

  fetch::math::Tensor<DataType> const tensor =
      fetch::math::Tensor<DataType>::FromString("1.1, 2.2, 3.3; 4.4, 5.5, 6.6;");

  // Index transposition table from [6,1] to [2,3]
  static std::map<SizeType, std::pair<SizeType, SizeType>> const TRANSPOSED_INDEXES = {
      {0, {0, 0}}, {1, {1, 0}}, {2, {0, 1}}, {3, {1, 1}}, {4, {0, 2}}, {5, {1, 2}},
  };

  // Initial shape of the Tensor is [2, 3]
  fetch::vm_modules::math::VMTensor vm_tensor(&toolkit.vm(), 0, tensor);

  Array<SizeType> e_shape(&toolkit.vm(), TypeIds::Unknown, TypeIds::Int32, int32_t(0));
  static std::vector<SizeType> const COMPATIBLE_SHAPE_RAW = {6, 1};
  for (SizeType dim_size : COMPATIBLE_SHAPE_RAW)
  {
    e_shape.Append(TemplateParameter1(dim_size, TypeIds::Int32));
  }
  auto const new_equal_shape = Ptr<IArray>::PtrFromThis(&e_shape);

  // Reshaping to an compatible shape should return TRUE;
  EXPECT_TRUE(vm_tensor.Reshape(new_equal_shape));

  fetch::math::Tensor<DataType> const reshaped = vm_tensor.GetTensor();

  // Assert the new shape is correct
  for (std::size_t i = 0; i < COMPATIBLE_SHAPE_RAW.size(); ++i)
  {
    EXPECT_EQ(reshaped.shape().at(i), COMPATIBLE_SHAPE_RAW.at(i));
  }

  // Assert all new elements are equal to initial ones and element indexes are transposed properly.
  for (SizeType i = 0; i < COMPATIBLE_SHAPE_RAW.at(0); ++i)
  {
    DataType const result = reshaped.At<fetch::math::SizeType, fetch::math::SizeType>(i, 0);

    std::pair<SizeType, SizeType> const index = TRANSPOSED_INDEXES.at(i);

    DataType const expected =
        tensor.At<fetch::math::SizeType, fetch::math::SizeType>(index.first, index.second);

    EXPECT_EQ(expected, result);
  }
}

TEST_F(MathTensorTests, tensor_argmax_test)
{
  static char const *tensor_argmax_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 2u64;

      var x = Tensor(tensor_shape);
      x.setAt(0u64, 0u64, 1.0fp64);
      x.setAt(0u64, 1u64, 2.0fp64);
      x.setAt(1u64, 0u64, 4.0fp64);
      x.setAt(1u64, 1u64, 3.0fp64);


      var ret_shape = Array<UInt64>(2);
      ret_shape[0] = 3u64;
      ret_shape[1] = 2u64;
      var ret = Tensor(ret_shape);

      var res1 = x.argMax();
      var res2 = x.argMax(0u64);
      var res3 = x.argMax(1u64);

      ret.setAt(0u64, 0u64, res1.at(0u64));
      ret.setAt(0u64, 1u64, res1.at(1u64));
      ret.setAt(1u64, 0u64, res2.at(0u64));
      ret.setAt(1u64, 1u64, res2.at(1u64));
      ret.setAt(2u64, 0u64, res3.at(0u64));
      ret.setAt(2u64, 1u64, res3.at(1u64));

      return ret;

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_argmax_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  fetch::math::Tensor<DataType> gt({3, 2});
  gt.Set(SizeType(0), SizeType(0), DataType{1});
  gt.Set(SizeType(0), SizeType(1), DataType{1});
  gt.Set(SizeType(1), SizeType(0), DataType{1});
  gt.Set(SizeType(1), SizeType(1), DataType{1});
  gt.Set(SizeType(2), SizeType(0), DataType{1});
  gt.Set(SizeType(2), SizeType(1), DataType{0});

  EXPECT_TRUE(gt.AllClose(tensor->GetTensor()));
}

TEST_F(MathTensorTests, tensor_dot_test)
{
  static char const *tensor_dot_src = R"(
    function main() : Tensor
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 2u64;

      var x = Tensor(tensor_shape);
      var y = Tensor(tensor_shape);
      x.fill(1.0fp64);
      y.fill(1.0fp64);

      var ret = x.dot(y);

      return ret;

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_dot_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  fetch::math::Tensor<DataType> gt({2, 2});
  gt.Fill(DataType{2});

  EXPECT_TRUE(gt.AllClose(tensor->GetTensor()));
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
  gt.Fill(fetch::math::Type<DataType>("7.0"));

  EXPECT_TRUE(gt.AllClose(tensor->GetTensor()));
}

TEST_F(MathTensorTests, tensor_reshape_from_string)
{
  static char const *SOURCE = R"(
      function main() : Tensor
        var tensor_shape = Array<UInt64>(3);
        tensor_shape[0] = 4u64;
        tensor_shape[1] = 1u64;
        tensor_shape[2] = 1u64;

        var x = Tensor(tensor_shape);
        x.fill(2.0fp64);

        var str_vals = "1.0, 1.0";
        x.fromString(str_vals);
        return x;
      endfunction
    )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run(&res));

  auto const                    tensor = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  fetch::math::Tensor<DataType> gt({2});
  gt.Fill(fetch::math::Type<DataType>("1.0"));

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

TEST_F(MathTensorTests, empty_tensor_shape)
{
  static char const *tensor_from_string_src = R"(
    function main()
      var x = Tensor();
      var shape = x.shape();
      print(shape);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_from_string_src));
  ASSERT_TRUE(toolkit.Run());
  ASSERT_EQ(stdout.str(), "[0]");
}

TEST_F(MathTensorTests, empty_tensor_size)
{
  static char const *tensor_from_string_src = R"(
    function main()
      var x = Tensor();
      var size = x.size();
      print(size);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_from_string_src));
  ASSERT_TRUE(toolkit.Run());
  ASSERT_EQ(stdout.str(), "0");
}

TEST_F(MathTensorTests, empty_tensor_from_string)
{
  static char const *tensor_from_string_src = R"(
    function main() : Tensor
      var x = Tensor();
      var string_vals = "1.0, 1.0, 1.0, 1.0";
      x.fromString(string_vals);
      return x;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_from_string_src));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const                    tensor = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  fetch::math::Tensor<DataType> gt({4, 1});
  gt.Fill(fetch::math::Type<DataType>("1.0"));

  EXPECT_TRUE(gt.AllClose(tensor->GetTensor()));
}

TEST_F(MathTensorTests, empty_tensor_fill)
{
  static char const *tensor_from_string_src = R"(
    function main()
      var x = Tensor();
      x.fill(5.0fp64);
      var shape = x.shape();
      print(shape);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_from_string_src));
  // does nothing because of size=0 and shape=[0]
  ASSERT_TRUE(toolkit.Run());
  ASSERT_EQ(stdout.str(), "[0]");
}

TEST_F(MathTensorTests, empty_tensor_random_fill)
{
  static char const *tensor_from_string_src = R"(
    function main()
      var x = Tensor();
      x.fillRandom();
      var shape = x.shape();
      print(shape);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_from_string_src));
  // does nothing because of size=0 and shape=[0]
  ASSERT_TRUE(toolkit.Run());
  ASSERT_EQ(stdout.str(), "[0]");
}

TEST_F(MathTensorTests, empty_tensor_reshape)
{
  static char const *tensor_from_string_src = R"(
    function main()
      var tensor_shape = Array<UInt64>(3);
      tensor_shape[0] = 4u64;
      tensor_shape[1] = 1u64;
      tensor_shape[2] = 1u64;

      var x = Tensor();
      x.reshape(tensor_shape);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_from_string_src));
  // impossible to reshape because shapes doesn't match
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(MathTensorTests, empty_tensor_unsqueeze)
{
  static char const *tensor_from_string_src = R"(
    function main()
      var x = Tensor();
      x = x.unsqueeze();
      var shape = x.shape();
      print(shape);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(tensor_from_string_src));
  ASSERT_TRUE(toolkit.Run());
  ASSERT_EQ(stdout.str(), "[0, 1]");
}

TEST_F(MathTensorTests, empty_tensor_at)
{
  static char const *src = R"(
    function main()
      var x = Tensor();
      x.at(0u64);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(src));
  ASSERT_FALSE(toolkit.Run());
}

}  // namespace
