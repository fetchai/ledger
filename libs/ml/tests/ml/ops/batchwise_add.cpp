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

#include "ml/ops/batchwise_add.hpp"

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <gtest/gtest.h>

template <typename T>
class BatchwiseAddTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(BatchwiseAddTest, MyTypes);

TYPED_TEST(BatchwiseAddTest, forward_test)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType height  = 7;
  SizeType width   = 6;
  SizeType batches = 5;

  TypeParam data1(std::vector<std::uint64_t>({height, width, batches}));
  TypeParam data2(std::vector<std::uint64_t>({height, width, 1}));
  TypeParam gt(std::vector<std::uint64_t>({height, width, batches}));

  for (SizeType i{0}; i < height; i++)
  {
    for (SizeType j{0}; j < width; j++)
    {
      for (SizeType n{0}; n < batches; n++)
      {
        data1(i, j, n) = static_cast<DataType>(i * 100 + j * 10 + n);
        gt(i, j, n)    = static_cast<DataType>((i * 100 + j * 10 + n) + (i * 100 + j * 10));
      }
      data2(i, j, 0) = static_cast<DataType>(i * 100 + j * 10);
    }
  }

  fetch::ml::ops::BatchwiseAdd<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({data1, data2}));
  op.Forward({data1, data2}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(BatchwiseAddTest, backward_test)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType length  = 5;
  SizeType batches = 7;

  TypeParam data1(std::vector<std::uint64_t>({length, batches}));
  TypeParam data2(std::vector<std::uint64_t>({length, 1}));
  TypeParam error_signal(data1.shape());
  TypeParam gt1(data1.shape());
  TypeParam gt2(data2.shape());

  for (SizeType i{0}; i < length; i++)
  {
    for (SizeType n{0}; n < batches; n++)
    {
      data1(i, n) = static_cast<DataType>(-1);
      data1(i, n) = static_cast<DataType>(-2);
      gt1(i, n)   = static_cast<DataType>(i * 10 + n);
      gt2(i, 0) += static_cast<DataType>(i * 10 + n);
      error_signal(i, n) = static_cast<DataType>(i * 10 + n);
    }
  }

  fetch::ml::ops::BatchwiseAdd<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({data1, data2}));
  op.Forward({data1, data2}, prediction);

  std::vector<TypeParam> gradients = op.Backward({data1, data2}, error_signal);

  // test correct values
  ASSERT_EQ(gradients.size(), 2);
  ASSERT_EQ(gradients[0].shape(), gt1.shape());
  ASSERT_EQ(gradients[1].shape(), gt2.shape());
  ASSERT_TRUE(gradients[0].AllClose(gt1));
  ASSERT_TRUE(gradients[1].AllClose(gt2));
}
