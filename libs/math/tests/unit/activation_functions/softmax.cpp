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
#include "math/activation_functions/softmax.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class SoftmaxTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SoftmaxTest, TensorFloatingTypes);

TYPED_TEST(SoftmaxTest, equal_proportion_test)
{
  using DataType = typename TypeParam::Type;

  std::size_t n = 1000;
  TypeParam   test_array{n};
  TypeParam   result_array{n};
  for (auto &e : test_array)
  {
    e = DataType{1};
  }

  fetch::math::Softmax(test_array, result_array);

  // check that all values equal proportion
  auto inv_n     = fetch::math::Type<DataType>("0.001");
  auto tolerance = fetch::math::function_tolerance<DataType>();
  EXPECT_NEAR(static_cast<double>(result_array[0]), static_cast<double>(inv_n),
              static_cast<double>(tolerance));
  for (std::size_t i = 1; i < n; ++i)
  {
    EXPECT_NEAR(static_cast<double>(result_array[i]), static_cast<double>(result_array[0]),
                static_cast<double>(tolerance));
  }
}

TYPED_TEST(SoftmaxTest, multi_dimension_test)
{
  using DataType = typename TypeParam::Type;

  TypeParam test_array = TypeParam::FromString("1, 2; 1, 4");
  test_array.Reshape({2, 2, 1});

  TypeParam gt_axis0 = TypeParam::FromString("0.5, 0.119202922; 0.5, 0.880797078");
  gt_axis0.Reshape({2, 2, 1});
  TypeParam gt_axis1 = TypeParam::FromString(
      "0.26894142137, 0.73105857863001;"
      "0.047425873177567, 0.95257412682243");
  gt_axis1.Reshape({2, 2, 1});

  TypeParam test_axis0({2, 2, 1}), test_axis1({2, 2, 1});
  fetch::math::Softmax(test_array, test_axis0, static_cast<typename TypeParam::SizeType>(0));
  fetch::math::Softmax(test_array, test_axis1, static_cast<typename TypeParam::SizeType>(1));

  // test correct values
  EXPECT_TRUE(test_axis0.AllClose(gt_axis0, fetch::math::function_tolerance<DataType>()));
  EXPECT_TRUE(
      test_axis1.AllClose(gt_axis1, DataType{2} * fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SoftmaxTest, exact_values_test)
{
  using DataType = typename TypeParam::Type;

  TypeParam test_array{8};
  TypeParam gt_array{8};

  test_array[0] = DataType{1};
  test_array[1] = DataType{-2};
  test_array[2] = DataType{3};
  test_array[3] = DataType{-4};
  test_array[4] = DataType{5};
  test_array[5] = DataType{-6};
  test_array[6] = DataType{7};
  test_array[7] = DataType{-8};

  gt_array[0] = fetch::math::Type<DataType>("0.002143744224529872770941886083651119");
  gt_array[1] = fetch::math::Type<DataType>("0.0001067307402698822468529838481590912");
  gt_array[2] = fetch::math::Type<DataType>("0.01584024633680981363097494317036258");
  gt_array[3] = fetch::math::Type<DataType>("0.00001444443496447785801762056106536456");
  gt_array[4] = fetch::math::Type<DataType>("0.1170444688035684441289369247679393");
  gt_array[5] = fetch::math::Type<DataType>("0.000001954841697110442501881410577271122");
  gt_array[6] = fetch::math::Type<DataType>("0.8648481460591056377393993328732979");
  gt_array[7] = fetch::math::Type<DataType>("0.0000002645590547611823744272849474530037");

  fetch::math::Softmax(test_array, test_array);

  ASSERT_TRUE(test_array.AllClose(gt_array, function_tolerance<DataType>(),
                                  function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
