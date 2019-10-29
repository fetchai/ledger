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

#include "math/activation_functions/softmax.hpp"
#include "math/tensor.hpp"

#include "gtest/gtest.h"

template <typename T>
class SoftmaxTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(SoftmaxTest, MyTypes);

TYPED_TEST(SoftmaxTest, equal_proportion_test)
{

  std::size_t n = 1000;
  TypeParam   test_array{n};
  TypeParam   result_array{n};
  for (auto &e : test_array)
  {
    e = typename TypeParam::Type(1);
  }

  fetch::math::Softmax(test_array, result_array);

  // check that all values equal proportion
  ASSERT_EQ(result_array[0], typename TypeParam::Type(1.0 / double(n)));
  for (std::size_t i = 1; i < n; ++i)
  {
    ASSERT_EQ(result_array[i], result_array[0]);
  }
}

TYPED_TEST(SoftmaxTest, multi_dimension_test)
{
  TypeParam test_array = TypeParam::FromString("1, 2; 1, 4");
  test_array.Reshape({2, 2, 1});

  TypeParam gt_axis0 = TypeParam::FromString("0.5, 0.119202922; 0.5, 0.880797078");
  gt_axis0.Reshape({2, 2, 1});
  TypeParam gt_axis1 = TypeParam::FromString("0.2689414, 0.7310586; 0.04742587, 0.9525741");
  gt_axis1.Reshape({2, 2, 1});

  TypeParam test_axis0({2, 2, 1}), test_axis1({2, 2, 1});
  fetch::math::Softmax(test_array, test_axis0, static_cast<typename TypeParam::SizeType>(0));
  fetch::math::Softmax(test_array, test_axis1, static_cast<typename TypeParam::SizeType>(1));

  // test correct values
  ASSERT_TRUE(
      test_axis0.AllClose(gt_axis0, fetch::math::function_tolerance<typename TypeParam::Type>()) ||
      test_axis0.AllClose(gt_axis0, static_cast<typename TypeParam::Type>(1e-5)));
  ASSERT_TRUE(
      test_axis1.AllClose(gt_axis1, fetch::math::function_tolerance<typename TypeParam::Type>()) ||
      test_axis0.AllClose(gt_axis0, static_cast<typename TypeParam::Type>(1e-5)));
}

TYPED_TEST(SoftmaxTest, exact_values_test)
{

  TypeParam test_array{8};
  TypeParam gt_array{8};

  test_array[0] = typename TypeParam::Type(1);
  test_array[1] = typename TypeParam::Type(-2);
  test_array[2] = typename TypeParam::Type(3);
  test_array[3] = typename TypeParam::Type(-4);
  test_array[4] = typename TypeParam::Type(5);
  test_array[5] = typename TypeParam::Type(-6);
  test_array[6] = typename TypeParam::Type(7);
  test_array[7] = typename TypeParam::Type(-8);

  gt_array[0] = typename TypeParam::Type(0.002143744224529872770941886083651119);
  gt_array[1] = typename TypeParam::Type(1.067307402698822468529838481590912E-4);
  gt_array[2] = typename TypeParam::Type(0.01584024633680981363097494317036258);
  gt_array[3] = typename TypeParam::Type(1.444443496447785801762056106536456E-5);
  gt_array[4] = typename TypeParam::Type(0.1170444688035684441289369247679393);
  gt_array[5] = typename TypeParam::Type(1.954841697110442501881410577271122E-6);
  gt_array[6] = typename TypeParam::Type(0.8648481460591056377393993328732979);
  gt_array[7] = typename TypeParam::Type(2.645590547611823744272849474530037E-7);

  fetch::math::Softmax(test_array, test_array);

  // test correct values
  for (std::size_t j = 0; j < gt_array.size(); ++j)
  {
    ASSERT_NEAR(double(test_array[j]), double(gt_array[j]), double(1e-7));
  }
}
