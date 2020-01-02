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

#include "gtest/gtest.h"
#include "math/activation_functions/sigmoid.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class SigmoidTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SigmoidTest, TensorFloatingTypes);

template <typename ArrayType>
ArrayType RandomArrayNegative(std::size_t n)
{
  using DataType = typename ArrayType::Type;

  ArrayType a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = random::Random::generator.AsType<DataType>() - DataType{1};
  }
  return a1;
}

template <typename ArrayType>
ArrayType RandomArrayPositive(std::size_t n)
{
  using DataType = typename ArrayType::Type;

  ArrayType a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = random::Random::generator.AsType<DataType>();
  }
  return a1;
}

TYPED_TEST(SigmoidTest, negative_response)
{
  using DataType = typename TypeParam::Type;

  std::size_t n          = 1000;
  TypeParam   test_array = RandomArrayNegative<TypeParam>(n);
  TypeParam   test_array_2{n};

  // sanity check that all values less than 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_LE(test_array[i], DataType{0});
  }

  //
  fetch::math::Sigmoid(test_array, test_array_2);

  // check that all values 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_LT(test_array_2[i], fetch::math::Type<DataType>("0.5"));
  }
}

TYPED_TEST(SigmoidTest, positive_response)
{
  using DataType = typename TypeParam::Type;

  std::size_t n          = 1000;
  TypeParam   test_array = RandomArrayPositive<TypeParam>(n);
  TypeParam   test_array_2{n};

  // sanity check that all values gte 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_GE(test_array[i], DataType{0});
  }

  fetch::math::Sigmoid(test_array, test_array_2);
  ASSERT_EQ(test_array.size(), test_array_2.size());
  ASSERT_EQ(test_array.shape(), test_array_2.shape());

  // check that all values unchanged
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_GE(test_array_2[i], fetch::math::Type<DataType>("0.5"));
  }
}

TYPED_TEST(SigmoidTest, exact_values)
{
  using DataType = typename TypeParam::Type;
  std::size_t n  = 8;
  TypeParam   test_array{n};
  TypeParam   gt_array{n};

  test_array[0] = DataType{1};
  test_array[1] = DataType{-2};
  test_array[2] = DataType{3};
  test_array[3] = DataType{-4};
  test_array[4] = DataType{5};
  test_array[5] = DataType{-6};
  test_array[6] = DataType{7};
  test_array[7] = DataType{-8};

  gt_array[0] = fetch::math::Type<DataType>("0.73105858");
  gt_array[1] = fetch::math::Type<DataType>("0.11920292");
  gt_array[2] = fetch::math::Type<DataType>("0.95257413");
  gt_array[3] = fetch::math::Type<DataType>("0.01798620996");
  gt_array[4] = fetch::math::Type<DataType>("0.993307149");
  gt_array[5] = fetch::math::Type<DataType>("0.002472623156635");
  gt_array[6] = fetch::math::Type<DataType>("0.999088948806");
  gt_array[7] = fetch::math::Type<DataType>("0.000335350130466");

  fetch::math::Sigmoid(test_array, test_array);
  ASSERT_EQ(test_array.size(), gt_array.size());
  ASSERT_EQ(test_array.shape(), gt_array.shape());

  // test correct values
  ASSERT_TRUE(test_array.AllClose(gt_array, function_tolerance<DataType>(),
                                  function_tolerance<DataType>()));
}

///////////////////
/// Sigmoid 2x2 ///
///////////////////
// Test sigmoid function output against numpy output for 2x2 input matrix of random values

TYPED_TEST(SigmoidTest, sigmoid_2x2)
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;
  TypeParam array1{{2, 2}};

  array1.Set(SizeType{0}, SizeType{0}, fetch::math::Type<DataType>("0.3"));
  array1.Set(SizeType{0}, SizeType{1}, fetch::math::Type<DataType>("1.2"));
  array1.Set(SizeType{1}, SizeType{0}, fetch::math::Type<DataType>("0.7"));
  array1.Set(SizeType{1}, SizeType{1}, DataType{22});

  TypeParam output = fetch::math::Sigmoid(array1);

  TypeParam numpy_output{{2, 2}};

  numpy_output.Set(SizeType{0}, SizeType{0}, fetch::math::Type<DataType>("0.57444252"));
  numpy_output.Set(SizeType{0}, SizeType{1}, fetch::math::Type<DataType>("0.76852478"));
  numpy_output.Set(SizeType{1}, SizeType{0}, fetch::math::Type<DataType>("0.66818777"));
  numpy_output.Set(SizeType{1}, SizeType{1}, DataType{1});
  ASSERT_TRUE(output.AllClose(numpy_output, fetch::math::function_tolerance<DataType>()));
}

///////////////////
/// Sigmoid 1x1 ///
///////////////////
// Test sigmoid function output against numpy output for 2x2 input matrix of random values
TYPED_TEST(SigmoidTest, sigmoid_11)
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  TypeParam input{1};
  TypeParam output{1};
  TypeParam numpy_output{1};

  input.Set(SizeType{0}, fetch::math::Type<DataType>("0.3"));
  numpy_output.Set(SizeType{0}, DataType{0});

  output = fetch::math::Sigmoid(input);

  numpy_output[0] = fetch::math::Type<DataType>("0.574442516811659");

  ASSERT_TRUE(output.AllClose(numpy_output, fetch::math::function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
