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

#include "ml/ops/flatten.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class FlattenTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(FlattenTest, MyTypes);

TYPED_TEST(FlattenTest, forward_test)
{
  TypeParam                          data(std::vector<std::uint64_t>({8, 8}));
  fetch::ml::ops::Flatten<TypeParam> op;
  TypeParam                          prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));

  // test correct values
  ASSERT_EQ(prediction.shape(), std::vector<typename TypeParam::SizeType>({1, 64}));
}

TYPED_TEST(FlattenTest, backward_test)
{
  TypeParam                          data(std::vector<std::uint64_t>({8, 8}));
  fetch::ml::ops::Flatten<TypeParam> op;
  TypeParam                          prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));
  TypeParam              errorSignal(prediction.shape());
  std::vector<TypeParam> gradients = op.Backward({data}, errorSignal);

  ASSERT_EQ(gradients.size(), 1);
  ASSERT_EQ(gradients[0].shape(), std::vector<typename TypeParam::SizeType>({8, 8}));
}

TYPED_TEST(FlattenTest, forward_batch_test)
{
  TypeParam data(std::vector<std::uint64_t>({5, 8, 8}));
  // Set incrementing values
  typename TypeParam::Type i(0);
  for (auto &e : data)
  {
    e = i;
    i += typename TypeParam::Type(1);
  }

  fetch::ml::ops::Flatten<TypeParam> op;
  TypeParam prediction = op.fetch::ml::template Ops<TypeParam>::ForwardBatch(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));

  // test correct shape
  ASSERT_EQ(prediction.shape(), std::vector<typename TypeParam::SizeType>({5, 1, 64}));

  // Make sure values are in order
  typename TypeParam::Type j(0);
  for (auto &e : prediction)
  {
    EXPECT_EQ(e, j);
    j += typename TypeParam::Type(1);
  }

  // Make sure input is left untouched
  typename TypeParam::Type k(0);
  for (auto &e : data)
  {
    EXPECT_EQ(e, k);
    k += typename TypeParam::Type(1);
  }
}

TYPED_TEST(FlattenTest, backward_batch_test)
{
  TypeParam data(std::vector<std::uint64_t>({5, 8, 8}));
  // Set incrementing values
  typename TypeParam::Type i(0);
  for (auto &e : data)
  {
    e = i;
    i += typename TypeParam::Type(1);
  }

  fetch::ml::ops::Flatten<TypeParam> op;
  TypeParam prediction = op.fetch::ml::template Ops<TypeParam>::ForwardBatch(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));
  std::vector<TypeParam> error_signal = op.fetch::ml::template BatchOps<TypeParam>::BackwardBatch(
      std::vector<std::reference_wrapper<TypeParam const>>({data}), prediction);

  EXPECT_EQ(error_signal.size(), 1);
  EXPECT_EQ(error_signal.front().shape(), data.shape());

  // Make sure values are in order
  typename TypeParam::Type j(0);
  for (auto &e : error_signal.front())
  {
    EXPECT_EQ(e, j);
    j += typename TypeParam::Type(1);
  }

  // Make sure input is left untouched
  typename TypeParam::Type k(0);
  for (auto &e : data)
  {
    EXPECT_EQ(e, k);
    k += typename TypeParam::Type(1);
  }
}
