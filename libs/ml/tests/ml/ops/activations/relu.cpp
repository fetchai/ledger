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

#include "core/serializers/byte_array_buffer.hpp"
#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "ml/ops/activation.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vectorise/fixed_point/serializers.hpp"

#include "gtest/gtest.h"

#include <vector>

template <typename T>
class ReluTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(ReluTest, MyTypes);

TYPED_TEST(ReluTest, forward_all_positive_test)
{
  using ArrayType = TypeParam;

  ArrayType data = ArrayType::FromString(R"(1, 2, 3, 4, 5, 6, 7, 8)");
  ArrayType gt   = ArrayType::FromString(R"(1, 2, 3, 4, 5, 6, 7, 8)");

  fetch::ml::ops::Relu<ArrayType> op;
  ArrayType prediction(op.ComputeOutputShape({std::make_shared<const ArrayType>(data)}));
  op.Forward({std::make_shared<const ArrayType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(ReluTest, forward_3d_tensor_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({2, 2, 2});
  ArrayType           gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> gt_input({1, 0, 3, 0, 5, 0, 7, 0});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      for (SizeType k{0}; k < 2; ++k)
      {
        data.Set(i, j, k, static_cast<DataType>(data_input[i + 2 * (j + 2 * k)]));
        gt.Set(i, j, k, static_cast<DataType>(gt_input[i + 2 * (j + 2 * k)]));
      }
    }
  }

  fetch::ml::ops::Relu<ArrayType> op;
  ArrayType prediction(op.ComputeOutputShape({std::make_shared<const ArrayType>(data)}));
  op.Forward({std::make_shared<const ArrayType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, static_cast<DataType>(1e-5), static_cast<DataType>(1e-5)));
}

TYPED_TEST(ReluTest, forward_all_negative_integer_test)
{
  using ArrayType = TypeParam;

  ArrayType data = ArrayType::FromString(R"(-1, -2, -3, -4, -5, -6, -7, -8)");
  ArrayType gt   = ArrayType::FromString(R"(0, 0, 0, 0, 0, 0, 0, 0)");

  fetch::ml::ops::Relu<ArrayType> op;
  ArrayType prediction(op.ComputeOutputShape({std::make_shared<const ArrayType>(data)}));
  op.Forward({std::make_shared<const ArrayType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(ReluTest, forward_mixed_test)
{
  using ArrayType = TypeParam;

  ArrayType data = ArrayType::FromString(R"(1, -2, 3, -4, 5, -6, 7, -8)");
  ArrayType gt   = ArrayType::FromString(R"(1, 0, 3, 0, 5, 0, 7, 0)");

  fetch::ml::ops::Relu<ArrayType> op;
  ArrayType prediction(op.ComputeOutputShape({std::make_shared<const ArrayType>(data)}));
  op.Forward({std::make_shared<const ArrayType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(ReluTest, backward_mixed_test)
{
  using ArrayType = TypeParam;

  ArrayType data  = ArrayType::FromString(R"(1, -2, 3, -4, 5, -6, 7, -8)");
  ArrayType error = ArrayType::FromString(R"(-1, 2, 3, -5, -8, 13, -21, -34)");
  ArrayType gt    = ArrayType::FromString(R"(-1, 0, 3, 0, -8, 0, -21, 0)");

  fetch::ml::ops::Relu<ArrayType> op;
  std::vector<ArrayType> prediction = op.Backward({std::make_shared<const ArrayType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt));
}

TYPED_TEST(ReluTest, backward_3d_tensor_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType        data({2, 2, 2});
  ArrayType        error({2, 2, 2});
  ArrayType        gt({2, 2, 2});
  std::vector<int> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> errorInput({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int> gt_input({-1, 0, 3, 0, -8, 0, -21, 0});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      for (SizeType k{0}; k < 2; ++k)
      {
        data.Set(i, j, k, static_cast<DataType>(data_input[i + 2 * (j + 2 * k)]));
        error.Set(i, j, k, static_cast<DataType>(errorInput[i + 2 * (j + 2 * k)]));
        gt.Set(i, j, k, static_cast<DataType>(gt_input[i + 2 * (j + 2 * k)]));
      }
    }
  }

  fetch::ml::ops::Relu<ArrayType> op;
  std::vector<ArrayType> prediction = op.Backward({std::make_shared<const ArrayType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, static_cast<DataType>(1e-5), static_cast<DataType>(1e-5)));
}

TYPED_TEST(ReluTest, saveparams_test)
{
  using ArrayType     = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TypeParam>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Relu<ArrayType>::SPType;
  using OpType        = typename fetch::ml::ops::Relu<ArrayType>;

  ArrayType data = ArrayType::FromString("1, 2, 3, 4, 5, 6, 7, 8");
  ArrayType gt   = ArrayType::FromString("1, 2, 3, 4, 5, 6, 7, 8");

  fetch::ml::ops::Relu<ArrayType> op;
  ArrayType                       prediction(op.ComputeOutputShape({data}));
  VecTensorType                   vec_data({data});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::SaveableParamsInterface> sp = op.GetOpSaveableParams();

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
