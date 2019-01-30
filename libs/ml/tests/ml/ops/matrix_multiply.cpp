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

#include "ml/ops/matrix_multiply.hpp"
#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class MatrixMultiplyTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<  // fetch::math::linalg::Matrix<int>,
    fetch::math::linalg::Matrix<float>, fetch::math::linalg::Matrix<double>,
    fetch::math::Tensor<int>, fetch::math::Tensor<float>, fetch::math::Tensor<double>,
    fetch::math::NDArray<int>, fetch::math::NDArray<float>, fetch::math::NDArray<double>>;
TYPED_TEST_CASE(MatrixMultiplyTest, MyTypes);

TYPED_TEST(MatrixMultiplyTest, forward_test)
{
  std::shared_ptr<TypeParam> a  = std::make_shared<TypeParam>(std::vector<std::size_t>({1, 5}));
  std::shared_ptr<TypeParam> b  = std::make_shared<TypeParam>(std::vector<std::size_t>({5, 4}));
  std::shared_ptr<TypeParam> gt = std::make_shared<TypeParam>(std::vector<std::size_t>({1, 4}));

  std::vector<int> data({1, 2, -3, 4, 5});
  std::vector<int> weights(
      {-11, 12, 13, 14, 21, 22, 23, 24, 31, 32, 33, 34, 41, 42, 43, 44, 51, 52, 53, 54});
  std::vector<int> results({357, 388, 397, 406});

  for (std::size_t i(0); i < data.size(); ++i)
  {
    a->Set(i, typename TypeParam::Type(data[i]));
  }
  for (std::size_t i(0); i < 5; ++i)
  {
    for (std::size_t j(0); j < 4; ++j)
    {
      b->Set(std::vector<std::size_t>({i, j}), typename TypeParam::Type(weights[i * 4 + j]));
    }
  }
  for (std::size_t i(0); i < results.size(); ++i)
  {
    gt->Set(i, typename TypeParam::Type(results[i]));
  }

  fetch::ml::ops::MatrixMultiply<TypeParam> op;
  std::shared_ptr<TypeParam>                prediction = op.Forward({a, b});

  // // test correct values
  ASSERT_TRUE(prediction->shape().size() == 2);
  ASSERT_TRUE(prediction->shape()[0] == 1);
  ASSERT_TRUE(prediction->shape()[1] == 4);
  ASSERT_TRUE(prediction->AllClose(*gt));
}

TYPED_TEST(MatrixMultiplyTest, backward_test)
{
  std::shared_ptr<TypeParam> a  = std::make_shared<TypeParam>(std::vector<std::size_t>({1, 5}));
  std::shared_ptr<TypeParam> b  = std::make_shared<TypeParam>(std::vector<std::size_t>({5, 4}));
  std::shared_ptr<TypeParam> e  = std::make_shared<TypeParam>(std::vector<std::size_t>({1, 4}));
  std::shared_ptr<TypeParam> ig = std::make_shared<TypeParam>(std::vector<std::size_t>({1, 5}));
  std::shared_ptr<TypeParam> wg = std::make_shared<TypeParam>(std::vector<std::size_t>({5, 4}));

  std::vector<int> data({1, 2, -3, 4, 5});
  std::vector<int> weights(
      {-11, 12, 13, 14, 21, 22, 23, 24, 31, 32, 33, 34, 41, 42, 43, 44, 51, 52, 53, 54});
  std::vector<int> errorSignal({1, 2, 3, -4});
  std::vector<int> inputGrad({-4, 38, 58, 78, 98});
  std::vector<int> weightsGrad(
      {1, 2, 3, -4, 2, 4, 6, -8, -3, -6, -9, 12, 4, 8, 12, -16, 5, 10, 15, -20});

  for (std::size_t i(0); i < data.size(); ++i)
  {
    a->Set(i, typename TypeParam::Type(data[i]));
  }
  for (std::size_t i(0); i < 5; ++i)
  {
    for (std::size_t j(0); j < 4; ++j)
    {
      b->Set(std::vector<std::size_t>({i, j}), typename TypeParam::Type(weights[i * 4 + j]));
    }
  }
  for (std::size_t i(0); i < errorSignal.size(); ++i)
  {
    e->Set(i, typename TypeParam::Type(errorSignal[i]));
  }
  for (std::size_t i(0); i < inputGrad.size(); ++i)
  {
    ig->Set(i, typename TypeParam::Type(inputGrad[i]));
  }
  for (std::size_t i(0); i < 5; ++i)
  {
    for (std::size_t j(0); j < 4; ++j)
    {
      wg->Set(std::vector<std::size_t>({i, j}), typename TypeParam::Type(weightsGrad[i * 4 + j]));
    }
  }

  fetch::ml::ops::MatrixMultiply<TypeParam> op;
  std::vector<std::shared_ptr<TypeParam>>   backpropagatedSignals =
      op.Backward(std::vector<std::shared_ptr<TypeParam>>({a, b}), e);

  // test correct shapes
  ASSERT_EQ(backpropagatedSignals.size(), 2);
  ASSERT_EQ(backpropagatedSignals[0]->shape().size(), 2);
  EXPECT_EQ(backpropagatedSignals[0]->shape()[0], 1);
  EXPECT_EQ(backpropagatedSignals[0]->shape()[1], 5);
  ASSERT_EQ(backpropagatedSignals[1]->shape().size(), 2);
  EXPECT_EQ(backpropagatedSignals[1]->shape()[0], 5);
  ASSERT_EQ(backpropagatedSignals[1]->shape()[1], 4);

  // test correct values
  EXPECT_TRUE(backpropagatedSignals[0]->AllClose(*ig));
  EXPECT_TRUE(backpropagatedSignals[1]->AllClose(*wg));
}
