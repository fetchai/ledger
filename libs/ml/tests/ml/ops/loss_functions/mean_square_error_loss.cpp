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

#include "math/tensor.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class MeanSquareErrorTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(MeanSquareErrorTest, MyTypes);

TYPED_TEST(MeanSquareErrorTest, perfect_match_forward_test)
{
  TypeParam     data1({8, 1});
  TypeParam     data2({8, 1});
  std::uint64_t i(0);
  for (int e : {1, -2, 3, -4, 5, -6, 7, -8})
  {
    data1.Set(i, 0, typename TypeParam::Type(e));
    data2.Set(i, 0, typename TypeParam::Type(e));
    i++;
  }

  fetch::ml::ops::MeanSquareErrorLoss<TypeParam> op;
  TypeParam                                      result({1, 1});
  op.Forward({data1, data2}, result);

  EXPECT_EQ(result(0, 0), typename TypeParam::Type(0));
}

TYPED_TEST(MeanSquareErrorTest, one_by_eight_dimensional_forward_test)
{
  TypeParam data1 = TypeParam::FromString("1.1; -2.2; 3.3; -4.4; 5.5; -6.6; 7.7; -8.8");
  TypeParam data2 = TypeParam::FromString("1.1; 2.2; 7.7; 6.6; 0.0; -6.6; 7.7; -9.9");

  TypeParam data1_transpose = data1.Transpose();
  TypeParam data2_transpose = data2.Transpose();

  fetch::ml::ops::MeanSquareErrorLoss<TypeParam> op;
  TypeParam                                      result({1, 1});
  op.Forward({data1_transpose, data2_transpose}, result);

  ASSERT_FLOAT_EQ(static_cast<float>(result(0, 0)), 191.18f / 8.0f / 2.0f);
  // fetch::math::MeanSquareErrorLoss divided sum by number of element (ie 8 in this case)
  // and then further divide by two (cf issue 343)
}

TYPED_TEST(MeanSquareErrorTest, one_by_eight_dimensional_backward_test)
{
  using DataType = typename TypeParam::Type;

  // first test 8 one-dimensional data points
  TypeParam data1 = TypeParam::FromString("1.1; -2.2; 3.3; -4.4; 5.5; -6.6; 7.7; -8.8");
  TypeParam data2 = TypeParam::FromString("1.1; 2.2; 7.7; 6.6; 0.0; -6.6; 7.7; -9.9");
  TypeParam gt    = TypeParam::FromString("0.0, -0.55, -0.55, -1.375, 0.6875, 0.0, 0.0, 0.1375");

  TypeParam data1_transpose = data1.Transpose();
  TypeParam data2_transpose = data2.Transpose();

  TypeParam error_signal({1, 1});
  error_signal(0, 0) = DataType{1};

  fetch::ml::ops::MeanSquareErrorLoss<TypeParam> op;
  std::vector<TypeParam> gradients = op.Backward({data1_transpose, data2_transpose}, error_signal);
  EXPECT_TRUE(
      gradients.at(0).AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>(),
                               fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(MeanSquareErrorTest, two_dimensional_forward_test_with_weighting)
{
  TypeParam data1 = TypeParam::FromString("1.1, -2.2, 3.3, -4.4; 5.5, -6.6, 7.7, -8.8");
  TypeParam data2 = TypeParam::FromString("1.1, 2.2, 7.7, 6.6; 0.0, -6.6, 7.7, -9.9");

  TypeParam weightings = TypeParam::FromString("1.0, 2.0, 1.0, 0.5; 0.0, 0.0, 0.0, 0.0");
  fetch::ml::ops::MeanSquareErrorLoss<TypeParam> op(weightings);
  TypeParam                                      result({1, 1});
  op.Forward({data1, data2}, result);

  ASSERT_FLOAT_EQ(static_cast<float>(result(0, 0)), 118.58f / 8.0f / 2.0f);
}

TYPED_TEST(MeanSquareErrorTest, two_dimensional_backward_test_with_weighting)
{
  TypeParam data1        = TypeParam::FromString("1.1, -2.2, 3.3, -4.4; 5.5, -6.6, 7.7, -8.8");
  TypeParam data2        = TypeParam::FromString("1.1, 2.2, 7.7, 6.6; 0.0, -6.6, 7.7, -9.9");
  TypeParam error_signal = TypeParam::FromString("0.1, 0.2, 0.7, 0.6; 0.0, 0.6, 0.7, 0.9");
  TypeParam weightings   = TypeParam::FromString("1.0, 2.0, 1.0, 0.5; 0.0, 0.0, 0.0, 0.0");
  TypeParam gt           = TypeParam::FromString("0.0, -2.2, -1.1, -1.375; 0.0, 0.0, 0.0, 0.0");

  fetch::ml::ops::MeanSquareErrorLoss<TypeParam> op(weightings);
  std::vector<TypeParam> gradients = op.Backward({data1, data2}, error_signal);

  EXPECT_TRUE(
      gradients.at(0).AllClose(gt, fetch::math::function_tolerance<typename TypeParam::Type>() * 4,
                               fetch::math::function_tolerance<typename TypeParam::Type>()) *
      4);
}
