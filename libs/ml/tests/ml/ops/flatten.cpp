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

#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "ml/ops/flatten.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vectorise/fixed_point/serializers.hpp"

#include "gtest/gtest.h"

#include <cstdint>

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
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType height  = 7;
  SizeType width   = 6;
  SizeType batches = 5;

  TypeParam data(std::vector<std::uint64_t>({height, width, batches}));
  TypeParam gt(std::vector<std::uint64_t>({height * width, batches}));

  for (SizeType i{0}; i < height; i++)
  {
    for (SizeType j{0}; j < width; j++)
    {
      for (SizeType n{0}; n < batches; n++)
      {
        data(i, j, n)         = static_cast<DataType>(i * 100 + j * 10 + n);
        gt(j * height + i, n) = static_cast<DataType>(i * 100 + j * 10 + n);
      }
    }
  }

  fetch::ml::ops::Flatten<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(FlattenTest, backward_test)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType height  = 5;
  SizeType width   = 6;
  SizeType batches = 7;

  TypeParam data({height, width, batches});
  TypeParam error_signal({height * width, batches});
  TypeParam gt(data.shape());

  for (SizeType i{0}; i < height; i++)
  {
    for (SizeType j{0}; j < width; j++)
    {
      for (SizeType n{0}; n < batches; n++)
      {
        data(i, j, n)                   = static_cast<DataType>(-1);
        gt(i, j, n)                     = static_cast<DataType>(i * 100 + j * 10 + n);
        error_signal(j * height + i, n) = static_cast<DataType>(i * 100 + j * 10 + n);
      }
    }
  }

  fetch::ml::ops::Flatten<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  std::vector<TypeParam> gradients = op.Backward({data}, error_signal);

  // test correct values
  ASSERT_EQ(gradients.size(), 1);
  ASSERT_EQ(gradients[0].shape(), gt.shape());
  ASSERT_TRUE(gradients[0].AllClose(gt));
}

TYPED_TEST(FlattenTest, saveparams_test)
{
  using ArrayType     = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::Ops<ArrayType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Flatten<ArrayType>::SPType;
  using OpType        = typename fetch::ml::ops::Flatten<ArrayType>;

  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType height  = 7;
  SizeType width   = 6;
  SizeType batches = 5;

  TypeParam data(std::vector<std::uint64_t>({height, width, batches}));
  TypeParam gt(std::vector<std::uint64_t>({height * width, batches}));

  for (SizeType i{0}; i < height; i++)
  {
    for (SizeType j{0}; j < width; j++)
    {
      for (SizeType n{0}; n < batches; n++)
      {
        data(i, j, n)         = static_cast<DataType>(i * 100 + j * 10 + n);
        gt(j * height + i, n) = static_cast<DataType>(i * 100 + j * 10 + n);
      }
    }
  }

  OpType op;

  ArrayType     prediction(op.ComputeOutputShape({data}));
  VecTensorType vec_data({data});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::SaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::ByteArrayBuffer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  ArrayType new_prediction(op.ComputeOutputShape({data}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, fetch::math::function_tolerance<DataType>(),
                                      fetch::math::function_tolerance<DataType>()));
}
