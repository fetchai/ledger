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

#include "math/base_types.hpp"
#include "ml/ops/reshape.hpp"

#include "gtest/gtest.h"
#include "test_types.hpp"

#include <vector>

namespace {

using SizeType = fetch::math::SizeType;

template <typename T>
class ReshapeTest : public ::testing::Test
{
};

TYPED_TEST_CASE(ReshapeTest, fetch::math::test::TensorFloatingTypes);

template <typename TensorType>
void ReshapeTestForward(std::vector<SizeType> const &initial_shape,
                        std::vector<SizeType> const &final_shape)
{
  TensorType                          data(initial_shape);
  TensorType                          gt(final_shape);
  fetch::ml::ops::Reshape<TensorType> op(final_shape);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  EXPECT_EQ(prediction.shape().size(), final_shape.size());
  for (std::size_t j = 0; j < final_shape.size(); ++j)
  {
    EXPECT_EQ(prediction.shape().at(j), final_shape.at(j));
  }
}

template <typename TensorType>
void ReshapeTestForwardWrong(std::vector<SizeType> const &initial_shape,
                             std::vector<SizeType> const &final_shape)
{
  TensorType                          data(initial_shape);
  TensorType                          gt(final_shape);
  fetch::ml::ops::Reshape<TensorType> op(final_shape);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  EXPECT_ANY_THROW(op.Forward({std::make_shared<const TensorType>(data)}, prediction));
}

template <typename TensorType>
void ReshapeTestBackward(std::vector<SizeType> const &initial_shape,
                         std::vector<SizeType> const &final_shape)
{

  TensorType data(initial_shape);
  TensorType error(final_shape);
  TensorType gt_error(initial_shape);

  data.FillUniformRandom();
  gt_error = data.Copy();

  // call reshape and store result in 'error'
  fetch::ml::ops::Reshape<TensorType>            op(final_shape);
  std::vector<std::shared_ptr<const TensorType>> data_vec({std::make_shared<TensorType>(data)});
  op.Forward(data_vec, error);

  // backprop with incoming error signal matching output data
  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  for (std::size_t j = 0; j < error_signal.at(0).shape().size(); ++j)
  {
    EXPECT_EQ(error_signal.at(0).shape().at(j), gt_error.shape().at(j));
  }

  EXPECT_TRUE(error_signal.at(0).AllClose(gt_error));
}

TYPED_TEST(ReshapeTest, forward_tests)
{
  ReshapeTestForward<TypeParam>({3, 2, 1}, {6, 1, 1});
  ReshapeTestForward<TypeParam>({6, 1, 2}, {3, 2, 2});
  ReshapeTestForward<TypeParam>({6, 1, 3}, {6, 1, 3});
  ReshapeTestForward<TypeParam>({6, 1, 4}, {6, 1, 1, 4});
  ReshapeTestForward<TypeParam>({3, 2, 5}, {6, 1, 1, 1, 5});

  ReshapeTestForward<TypeParam>({3, 2, 1}, {6, 1, 1});
  ReshapeTestForward<TypeParam>({6, 1, 1, 2}, {6, 1, 2});
  ReshapeTestForward<TypeParam>({6, 1, 1, 1, 3}, {3, 2, 3});

  ReshapeTestForward<TypeParam>({7, 6, 5, 4, 3, 2, 1, 1}, {7, 6, 5, 4, 3, 2, 1});
  ReshapeTestForward<TypeParam>({1, 2, 3, 4, 5, 6, 7, 2}, {7, 6, 5, 4, 3, 2, 1, 2});
  ReshapeTestForward<TypeParam>({1, 2, 3, 4, 5, 6, 7, 3}, {5040, 1, 1, 1, 1, 3});
}

TYPED_TEST(ReshapeTest, forward_wrong_tests)
{
  ReshapeTestForwardWrong<TypeParam>({3, 4}, {6, 1});
  ReshapeTestForwardWrong<TypeParam>({6, 2, 1}, {6, 1});
  ReshapeTestForwardWrong<TypeParam>({7, 6, 5, 4, 3, 2, 1}, {7, 6, 5, 1});
  ReshapeTestForwardWrong<TypeParam>({3, 4, 1}, {6, 1, 1});
  ReshapeTestForwardWrong<TypeParam>({6, 1, 2, 1}, {6, 1, 1});
  ReshapeTestForwardWrong<TypeParam>({7, 6, 5, 4, 3, 2, 1, 1}, {7, 6, 5, 1});
}

TYPED_TEST(ReshapeTest, backward_tests)
{
  ReshapeTestBackward<TypeParam>({3, 2, 5}, {6, 1, 5});
  ReshapeTestBackward<TypeParam>({6, 1, 6}, {3, 2, 6});
  ReshapeTestBackward<TypeParam>({6, 1, 7}, {6, 1, 7});
  ReshapeTestBackward<TypeParam>({6, 1, 8}, {6, 1, 1, 8});
  ReshapeTestBackward<TypeParam>({3, 2, 9}, {6, 1, 1, 1, 9});

  ReshapeTestBackward<TypeParam>({3, 2, 2}, {6, 1, 2});
  ReshapeTestBackward<TypeParam>({6, 1, 1, 3}, {6, 1, 3});
  ReshapeTestBackward<TypeParam>({6, 1, 1, 1, 4}, {3, 2, 4});

  ReshapeTestBackward<TypeParam>({7, 6, 5, 4, 3, 2, 1, 7}, {7, 6, 5, 4, 3, 2, 7});
  ReshapeTestBackward<TypeParam>({1, 2, 3, 4, 5, 6, 7, 3}, {7, 6, 5, 4, 3, 2, 1, 3});
  ReshapeTestBackward<TypeParam>({1, 2, 3, 4, 5, 6, 7, 1}, {5040, 1, 1, 1, 1, 1});
}

}  // namespace
