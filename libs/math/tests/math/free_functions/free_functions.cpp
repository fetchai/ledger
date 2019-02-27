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

#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

#include "core/random/lcg.hpp"
#include "math/free_functions/free_functions.hpp"

#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"

template <typename T>
class FreeFunctionsTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(FreeFunctionsTest, MyTypes);

///////////////////
/// Sigmoid 2x2 ///
///////////////////
// Test sigmoid function output against numpy output for 2x2 input matrix of random values

TYPED_TEST(FreeFunctionsTest, sigmoid_2x2)
{
  TypeParam array1{{2, 2}};

  array1.Set(0, typename TypeParam::Type(0.3));
  array1.Set(1, typename TypeParam::Type(1.2));
  array1.Set(2, typename TypeParam::Type(0.7));
  array1.Set(3, typename TypeParam::Type(22));

  TypeParam output = fetch::math::Sigmoid(array1);

  TypeParam numpy_output{{2, 2}};

  numpy_output.Set(0, typename TypeParam::Type(0.57444252));
  numpy_output.Set(1, typename TypeParam::Type(0.76852478));
  numpy_output.Set(2, typename TypeParam::Type(0.66818777));
  numpy_output.Set(3, typename TypeParam::Type(1));

  ASSERT_TRUE(output.AllClose(numpy_output));
}

///////////////////
/// Sigmoid 1x1 ///
///////////////////
// Test sigmoid function output against numpy output for 2x2 input matrix of random values
TYPED_TEST(FreeFunctionsTest, sigmoid_11)
{
  TypeParam input{1};
  TypeParam output{1};
  TypeParam numpy_output{1};

  input.Set(0, typename TypeParam::Type(0.3));
  numpy_output.Set(0, typename TypeParam::Type(0));

  output = fetch::math::Sigmoid(input);

  numpy_output[0] = typename TypeParam::Type(0.574442516811659);

  ASSERT_TRUE(output.AllClose(numpy_output));
}

////////////////
/// Tanh 2x2 ///
////////////////
// Test sigmoid function output against numpy output for 2x2 input matrix of random values

TYPED_TEST(FreeFunctionsTest, tanh_22)
{
  TypeParam array1{{2, 2}};

  array1.Set(0, typename TypeParam::Type(0.3));
  array1.Set(1, typename TypeParam::Type(1.2));
  array1.Set(2, typename TypeParam::Type(0.7));
  array1.Set(3, typename TypeParam::Type(22));

  TypeParam output = array1;
  fetch::math::Tanh(output);

  TypeParam numpy_output{{2, 2}};

  numpy_output.Set(0, typename TypeParam::Type(0.29131261));
  numpy_output.Set(1, typename TypeParam::Type(0.83365461));
  numpy_output.Set(2, typename TypeParam::Type(0.60436778));
  numpy_output.Set(3, typename TypeParam::Type(1));

  ASSERT_TRUE(output.AllClose(numpy_output));
}