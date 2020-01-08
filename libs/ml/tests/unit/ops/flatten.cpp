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

#include "core/serializers/main_serializer_definition.hpp"
#include "math/base_types.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class FlattenTest : public ::testing::Test
{
};

TYPED_TEST_CASE(FlattenTest, math::test::TensorIntAndFloatingTypes);

TYPED_TEST(FlattenTest, forward_test)
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType height  = 7;
  SizeType width   = 6;
  SizeType batches = 5;

  TypeParam data(std::vector<uint64_t>({height, width, batches}));
  TypeParam gt(std::vector<uint64_t>({height * width, batches}));

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

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TypeParam>(data)}));
  op.Forward({std::make_shared<const TypeParam>(data)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(FlattenTest, backward_test)
{
  using SizeType = fetch::math::SizeType;
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

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TypeParam>(data)}));
  op.Forward({std::make_shared<const TypeParam>(data)}, prediction);

  std::vector<TypeParam> gradients =
      op.Backward({std::make_shared<const TypeParam>(data)}, error_signal);

  // test correct values
  ASSERT_EQ(gradients.size(), 1);
  ASSERT_EQ(gradients[0].shape(), gt.shape());
  ASSERT_TRUE(gradients[0].AllClose(gt));
}

TYPED_TEST(FlattenTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Flatten<TensorType>::SPType;
  using OpType        = fetch::ml::ops::Flatten<TensorType>;

  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType height  = 7;
  SizeType width   = 6;
  SizeType batches = 5;

  TypeParam data(std::vector<uint64_t>({height, width, batches}));
  TypeParam gt(std::vector<uint64_t>({height * width, batches}));

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

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(FlattenTest, saveparams_backward_test)
{
  using SizeType   = fetch::math::SizeType;
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using OpType     = fetch::ml::ops::Flatten<TensorType>;
  using SPType     = typename OpType::SPType;

  SizeType height  = 5;
  SizeType width   = 6;
  SizeType batches = 7;

  TypeParam data({height, width, batches});
  TypeParam error_signal({height * width, batches});

  for (SizeType i{0}; i < height; i++)
  {
    for (SizeType j{0}; j < width; j++)
    {
      for (SizeType n{0}; n < batches; n++)
      {
        data(i, j, n)                   = static_cast<DataType>(-1);
        error_signal(j * height + i, n) = static_cast<DataType>(i * 100 + j * 10 + n);
      }
    }
  }

  fetch::ml::ops::Flatten<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TypeParam>(data)}));
  op.Forward({std::make_shared<const TypeParam>(data)}, prediction);

  std::vector<TypeParam> gradients =
      op.Backward({std::make_shared<const TypeParam>(data)}, error_signal);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  gradients = op.Backward({std::make_shared<const TypeParam>(data)}, error_signal);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TypeParam> new_gradients =
      new_op.Backward({std::make_shared<const TypeParam>(data)}, error_signal);

  // test correct values
  EXPECT_TRUE(gradients.at(0).AllClose(
      new_gradients.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
