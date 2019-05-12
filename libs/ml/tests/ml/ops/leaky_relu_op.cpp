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

#include "ml/ops/leaky_relu_op.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class LeakyReluOpTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(LeakyReluOpTest, MyTypes);

TYPED_TEST(LeakyReluOpTest, forward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType           data(8);
  ArrayType           alpha(8);
  ArrayType           gt(8);
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> alphaInput({0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8});
  std::vector<double> gt_input({1, -0.4, 3, -1.6, 5, -3.6, 7, -6.4});
  for (std::uint64_t i(0); i < 8; ++i)
  {
    data.Set(i, DataType(data_input[i]));
    gt.Set(i, DataType(gt_input[i]));
    alpha.Set(i, DataType(alphaInput[i]));
  }
  fetch::ml::ops::LeakyReluOp<ArrayType> op;
  ArrayType prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data, alpha}));

  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(LeakyReluOpTest, backward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType           data(8);
  ArrayType           alpha(8);
  ArrayType           error(8);
  ArrayType           gt(8);
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> alphaInput({0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8});
  std::vector<double> errorInput({0, 0, 0, 0, 1, 1, 0, 0});
  std::vector<double> gt_input({0, 0, 0, 0, 1, 0.6, 0, 0});
  for (std::uint64_t i(0); i < 8; ++i)
  {
    data.Set(i, DataType(data_input[i]));
    error.Set(i, DataType(errorInput[i]));
    gt.Set(i, DataType(gt_input[i]));
    alpha.Set(i, DataType(alphaInput[i]));
  }
  fetch::ml::ops::LeakyReluOp<ArrayType> op;
  std::vector<ArrayType>                 prediction = op.Backward({data, alpha}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType(1e-5), DataType(1e-5)));
}
