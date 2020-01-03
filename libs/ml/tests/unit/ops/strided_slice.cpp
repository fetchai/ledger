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

#include "ml/ops/strided_slice.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class StridedSliceTest : public ::testing::Test
{
};

TYPED_TEST_CASE(StridedSliceTest, math::test::TensorIntAndFloatingTypes);

TYPED_TEST(StridedSliceTest, forward_1D_test)
{
  using SizeVector = typename TypeParam::SizeVector;
  using SizeType   = fetch::math::SizeType;
  using DataType   = typename TypeParam::Type;

  TypeParam input({9});
  TypeParam gt({6});

  SizeVector begins({3});
  SizeVector ends({8});
  SizeVector strides({1});

  auto     it  = input.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < gt.shape().at(0); i++)
  {
    gt.At(i) = input.At(begins.at(0) + i * strides.at(0));
  }

  fetch::ml::ops::StridedSlice<TypeParam> op(begins, ends, strides);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  op.Forward({std::make_shared<TypeParam>(input)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(StridedSliceTest, backward_1D_test)
{
  using SizeType   = fetch::math::SizeType;
  using SizeVector = typename TypeParam::SizeVector;
  using DataType   = typename TypeParam::Type;

  TypeParam input({9});
  TypeParam error({6});
  TypeParam gt({9});

  SizeVector begins({3});
  SizeVector ends({8});
  SizeVector strides({1});

  auto     it  = error.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < error.shape().at(0); i++)
  {
    gt.At(begins.at(0) + i * strides.at(0)) = error.At(i);
  }

  fetch::ml::ops::StridedSlice<TypeParam> op(begins, ends, strides);
  // run backward twice to make sure the buffering is working
  op.Backward({std::make_shared<TypeParam>(input)}, error);
  std::vector<TypeParam> backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(input)}, error);

  // test correct shapes
  ASSERT_EQ(backpropagated_signals.size(), 1);
  ASSERT_EQ(backpropagated_signals[0].shape(), input.shape());

  // test correct values
  EXPECT_TRUE(backpropagated_signals[0].AllClose(gt));
}

TYPED_TEST(StridedSliceTest, forward_2D_test)
{
  using SizeVector = typename TypeParam::SizeVector;
  using SizeType   = fetch::math::SizeType;
  using DataType   = typename TypeParam::Type;

  TypeParam input({9, 9});
  TypeParam gt({6, 4});

  SizeVector begins({3, 1});
  SizeVector ends({8, 7});
  SizeVector strides({1, 2});

  auto     it  = input.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < gt.shape().at(0); i++)
  {
    for (SizeType j{0}; j < gt.shape().at(1); j++)
    {
      gt.At(i, j) = input.At(begins.at(0) + i * strides.at(0), begins.at(1) + j * strides.at(1));
    }
  }

  fetch::ml::ops::StridedSlice<TypeParam> op(begins, ends, strides);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  op.Forward({std::make_shared<TypeParam>(input)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(StridedSliceTest, backward_2D_test)
{
  using SizeType   = fetch::math::SizeType;
  using SizeVector = typename TypeParam::SizeVector;
  using DataType   = typename TypeParam::Type;

  TypeParam input({9, 9});
  TypeParam error({6, 4});
  TypeParam gt({9, 9});

  SizeVector begins({3, 1});
  SizeVector ends({8, 7});
  SizeVector strides({1, 2});

  auto     it  = error.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < error.shape().at(0); i++)
  {
    for (SizeType j{0}; j < error.shape().at(1); j++)
    {
      gt.At(begins.at(0) + i * strides.at(0), begins.at(1) + j * strides.at(1)) = error.At(i, j);
    }
  }

  fetch::ml::ops::StridedSlice<TypeParam> op(begins, ends, strides);
  // run backward twice to make sure the buffering is working
  op.Backward({std::make_shared<TypeParam>(input)}, error);
  std::vector<TypeParam> backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(input)}, error);

  // test correct shapes
  ASSERT_EQ(backpropagated_signals.size(), 1);
  ASSERT_EQ(backpropagated_signals[0].shape(), input.shape());

  // test correct values
  EXPECT_TRUE(backpropagated_signals[0].AllClose(gt));
}

TYPED_TEST(StridedSliceTest, forward_3D_test)
{
  using SizeVector = typename TypeParam::SizeVector;
  using SizeType   = fetch::math::SizeType;
  using DataType   = typename TypeParam::Type;

  TypeParam input({9, 9, 9});
  TypeParam gt({6, 4, 3});

  SizeVector begins({3, 1, 0});
  SizeVector ends({8, 7, 8});
  SizeVector strides({1, 2, 3});

  auto     it  = input.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < gt.shape().at(0); i++)
  {
    for (SizeType j{0}; j < gt.shape().at(1); j++)
    {
      for (SizeType k{0}; k < gt.shape().at(2); k++)
      {
        gt.At(i, j, k) =
            input.At(begins.at(0) + i * strides.at(0), begins.at(1) + j * strides.at(1),
                     begins.at(2) + k * strides.at(2));
      }
    }
  }

  fetch::ml::ops::StridedSlice<TypeParam> op(begins, ends, strides);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  op.Forward({std::make_shared<TypeParam>(input)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(StridedSliceTest, backward_3D_test)
{
  using SizeType   = fetch::math::SizeType;
  using SizeVector = typename TypeParam::SizeVector;
  using DataType   = typename TypeParam::Type;

  TypeParam input({9, 9, 9});
  TypeParam error({6, 4, 3});
  TypeParam gt({9, 9, 9});

  SizeVector begins({3, 1, 0});
  SizeVector ends({8, 7, 8});
  SizeVector strides({1, 2, 3});

  auto     it  = error.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < error.shape().at(0); i++)
  {
    for (SizeType j{0}; j < error.shape().at(1); j++)
    {
      for (SizeType k{0}; k < error.shape().at(2); k++)
      {
        gt.At(begins.at(0) + i * strides.at(0), begins.at(1) + j * strides.at(1),
              begins.at(2) + k * strides.at(2)) = error.At(i, j, k);
      }
    }
  }

  fetch::ml::ops::StridedSlice<TypeParam> op(begins, ends, strides);
  // run backward twice to make sure the buffering is working
  op.Backward({std::make_shared<TypeParam>(input)}, error);
  std::vector<TypeParam> backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(input)}, error);

  // test correct shapes
  ASSERT_EQ(backpropagated_signals.size(), 1);
  ASSERT_EQ(backpropagated_signals[0].shape(), input.shape());

  // test correct values
  EXPECT_TRUE(backpropagated_signals[0].AllClose(gt));
}

TYPED_TEST(StridedSliceTest, forward_4D_test)
{
  using SizeVector = typename TypeParam::SizeVector;
  using SizeType   = fetch::math::SizeType;
  using DataType   = typename TypeParam::Type;

  TypeParam input({9, 9, 9, 6});
  TypeParam gt({6, 4, 3, 1});

  SizeVector begins({3, 1, 0, 4});
  SizeVector ends({8, 7, 8, 5});
  SizeVector strides({1, 2, 3, 4});

  auto     it  = input.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < gt.shape().at(0); i++)
  {
    for (SizeType j{0}; j < gt.shape().at(1); j++)
    {
      for (SizeType k{0}; k < gt.shape().at(2); k++)
      {
        for (SizeType l{0}; l < gt.shape().at(3); l++)
        {
          gt.At(i, j, k, l) =
              input.At(begins.at(0) + i * strides.at(0), begins.at(1) + j * strides.at(1),
                       begins.at(2) + k * strides.at(2), begins.at(3) + l * strides.at(3));
        }
      }
    }
  }

  fetch::ml::ops::StridedSlice<TypeParam> op(begins, ends, strides);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  op.Forward({std::make_shared<TypeParam>(input)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(StridedSliceTest, backward_4D_test)
{
  using SizeType   = fetch::math::SizeType;
  using SizeVector = typename TypeParam::SizeVector;
  using DataType   = typename TypeParam::Type;

  TypeParam input({9, 9, 9, 6});
  TypeParam error({6, 4, 3, 1});
  TypeParam gt({9, 9, 9, 6});

  SizeVector begins({3, 1, 0, 4});
  SizeVector ends({8, 7, 8, 5});
  SizeVector strides({1, 2, 3, 4});

  auto     it  = error.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < error.shape().at(0); i++)
  {
    for (SizeType j{0}; j < error.shape().at(1); j++)
    {
      for (SizeType k{0}; k < error.shape().at(2); k++)
      {
        for (SizeType l{0}; l < error.shape().at(3); l++)
        {
          gt.At(begins.at(0) + i * strides.at(0), begins.at(1) + j * strides.at(1),
                begins.at(2) + k * strides.at(2), begins.at(3) + l * strides.at(3)) =
              error.At(i, j, k, l);
        }
      }
    }
  }

  fetch::ml::ops::StridedSlice<TypeParam> op(begins, ends, strides);
  // run backward twice to make sure the buffering is working
  op.Backward({std::make_shared<TypeParam>(input)}, error);
  std::vector<TypeParam> backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(input)}, error);

  // test correct shapes
  ASSERT_EQ(backpropagated_signals.size(), 1);
  ASSERT_EQ(backpropagated_signals[0].shape(), input.shape());

  // test correct values
  EXPECT_TRUE(backpropagated_signals[0].AllClose(gt));
}

TYPED_TEST(StridedSliceTest, forward_5D_test)
{
  using SizeVector = typename TypeParam::SizeVector;
  using SizeType   = fetch::math::SizeType;
  using DataType   = typename TypeParam::Type;

  TypeParam input({9, 9, 9, 6, 4});
  TypeParam gt({6, 4, 3, 1, 2});

  SizeVector begins({3, 1, 0, 4, 0});
  SizeVector ends({8, 7, 8, 5, 2});
  SizeVector strides({1, 2, 3, 4, 2});

  auto     it  = input.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < gt.shape().at(0); i++)
  {
    for (SizeType j{0}; j < gt.shape().at(1); j++)
    {
      for (SizeType k{0}; k < gt.shape().at(2); k++)
      {
        for (SizeType l{0}; l < gt.shape().at(3); l++)
        {
          for (SizeType m{0}; m < gt.shape().at(4); m++)
          {
            gt.At(i, j, k, l, m) =
                input.At(begins.at(0) + i * strides.at(0), begins.at(1) + j * strides.at(1),
                         begins.at(2) + k * strides.at(2), begins.at(3) + l * strides.at(3),
                         begins.at(4) + m * strides.at(4));
          }
        }
      }
    }
  }

  fetch::ml::ops::StridedSlice<TypeParam> op(begins, ends, strides);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  op.Forward({std::make_shared<TypeParam>(input)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(StridedSliceTest, backward_5D_test)
{
  using SizeType   = fetch::math::SizeType;
  using SizeVector = typename TypeParam::SizeVector;
  using DataType   = typename TypeParam::Type;

  TypeParam input({9, 9, 9, 6, 4});
  TypeParam error({6, 4, 3, 1, 2});
  TypeParam gt({9, 9, 9, 6, 4});

  SizeVector begins({3, 1, 0, 4, 0});
  SizeVector ends({8, 7, 8, 5, 2});
  SizeVector strides({1, 2, 3, 4, 2});

  auto     it  = error.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < error.shape().at(0); i++)
  {
    for (SizeType j{0}; j < error.shape().at(1); j++)
    {
      for (SizeType k{0}; k < error.shape().at(2); k++)
      {
        for (SizeType l{0}; l < error.shape().at(3); l++)
        {
          for (SizeType m{0}; m < error.shape().at(4); m++)
          {

            gt.At(begins.at(0) + i * strides.at(0), begins.at(1) + j * strides.at(1),
                  begins.at(2) + k * strides.at(2), begins.at(3) + l * strides.at(3),
                  begins.at(4) + m * strides.at(4)) = error.At(i, j, k, l, m);
          }
        }
      }
    }
  }

  fetch::ml::ops::StridedSlice<TypeParam> op(begins, ends, strides);
  // run backward twice to make sure the buffering is working
  op.Backward({std::make_shared<TypeParam>(input)}, error);
  std::vector<TypeParam> backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(input)}, error);

  // test correct shapes
  ASSERT_EQ(backpropagated_signals.size(), 1);
  ASSERT_EQ(backpropagated_signals[0].shape(), input.shape());

  // test correct values
  EXPECT_TRUE(backpropagated_signals[0].AllClose(gt));
}

TYPED_TEST(StridedSliceTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using SizeType      = fetch::math::SizeType;
  using SizeVector    = typename TypeParam::SizeVector;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::StridedSlice<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::StridedSlice<TensorType>;

  TypeParam input({9, 9, 9, 6, 4});
  TypeParam gt({6, 4, 3, 1, 2});

  SizeVector begins({3, 1, 0, 4, 0});
  SizeVector ends({8, 7, 8, 5, 2});
  SizeVector strides({1, 2, 3, 4, 2});

  auto     it  = input.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < gt.shape().at(0); i++)
  {
    for (SizeType j{0}; j < gt.shape().at(1); j++)
    {
      for (SizeType k{0}; k < gt.shape().at(2); k++)
      {
        for (SizeType l{0}; l < gt.shape().at(3); l++)
        {
          for (SizeType m{0}; m < gt.shape().at(4); m++)
          {
            gt.At(i, j, k, l, m) =
                input.At(begins.at(0) + i * strides.at(0), begins.at(1) + j * strides.at(1),
                         begins.at(2) + k * strides.at(2), begins.at(3) + l * strides.at(3),
                         begins.at(4) + m * strides.at(4));
          }
        }
      }
    }
  }

  OpType op(begins, ends, strides);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(input)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(input)});

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
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(input)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(StridedSliceTest, saveparams_backward_batch_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::StridedSlice<TensorType>;
  using SPType     = typename OpType::SPType;
  using SizeType   = fetch::math::SizeType;
  using SizeVector = typename TypeParam::SizeVector;
  using DataType   = typename TypeParam::Type;

  TypeParam input({9, 9, 9, 6, 4});
  TypeParam error({6, 4, 3, 1, 2});
  TypeParam gt({9, 9, 9, 6, 4});

  SizeVector begins({3, 1, 0, 4, 0});
  SizeVector ends({8, 7, 8, 5, 2});
  SizeVector strides({1, 2, 3, 4, 2});

  auto     it  = error.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < error.shape().at(0); i++)
  {
    for (SizeType j{0}; j < error.shape().at(1); j++)
    {
      for (SizeType k{0}; k < error.shape().at(2); k++)
      {
        for (SizeType l{0}; l < error.shape().at(3); l++)
        {
          for (SizeType m{0}; m < error.shape().at(4); m++)
          {

            gt.At(begins.at(0) + i * strides.at(0), begins.at(1) + j * strides.at(1),
                  begins.at(2) + k * strides.at(2), begins.at(3) + l * strides.at(3),
                  begins.at(4) + m * strides.at(4)) = error.At(i, j, k, l, m);
          }
        }
      }
    }
  }

  fetch::ml::ops::StridedSlice<TypeParam> op(begins, ends, strides);
  std::vector<TypeParam>                  backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(input)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  backpropagated_signals = op.Backward({std::make_shared<TypeParam>(input)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TypeParam> new_backpropagated_signals =
      new_op.Backward({std::make_shared<TypeParam>(input)}, error);

  // test correct values
  EXPECT_TRUE(backpropagated_signals.at(0).AllClose(
      new_backpropagated_signals.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
