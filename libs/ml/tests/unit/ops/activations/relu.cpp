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
#include "gtest/gtest.h"
#include "math/base_types.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class ReluTest : public ::testing::Test
{
};

TYPED_TEST_CASE(ReluTest, math::test::TensorFloatingTypes);

TYPED_TEST(ReluTest, forward_all_positive_test)
{
  using TensorType = TypeParam;

  TensorType data = TensorType::FromString(R"(1, 2, 3, 4, 5, 6, 7, 8)");
  TensorType gt   = TensorType::FromString(R"(1, 2, 3, 4, 5, 6, 7, 8)");

  fetch::ml::ops::Relu<TensorType> op;
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(ReluTest, forward_3d_tensor_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({2, 2, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> gt_input({1, 0, 3, 0, 5, 0, 7, 0});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      for (SizeType k{0}; k < 2; ++k)
      {
        data.Set(i, j, k, fetch::math::AsType<DataType>(data_input[i + 2 * (j + 2 * k)]));
        gt.Set(i, j, k, fetch::math::AsType<DataType>(gt_input[i + 2 * (j + 2 * k)]));
      }
    }
  }

  fetch::ml::ops::Relu<TensorType> op;
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::Type<DataType>("0.00001"),
                                  fetch::math::Type<DataType>("0.00001")));
}

TYPED_TEST(ReluTest, forward_all_negative_integer_test)
{
  using TensorType = TypeParam;

  TensorType data = TensorType::FromString(R"(-1, -2, -3, -4, -5, -6, -7, -8)");
  TensorType gt   = TensorType::FromString(R"(0, 0, 0, 0, 0, 0, 0, 0)");

  fetch::ml::ops::Relu<TensorType> op;
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(ReluTest, forward_mixed_test)
{
  using TensorType = TypeParam;

  TensorType data = TensorType::FromString(R"(1, -2, 3, -4, 5, -6, 7, -8)");
  TensorType gt   = TensorType::FromString(R"(1, 0, 3, 0, 5, 0, 7, 0)");

  fetch::ml::ops::Relu<TensorType> op;
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(ReluTest, backward_mixed_test)
{
  using TensorType = TypeParam;

  TensorType data  = TensorType::FromString(R"(1, -2, 3, -4, 5, -6, 7, -8)");
  TensorType error = TensorType::FromString(R"(-1, 2, 3, -5, -8, 13, -21, -34)");
  TensorType gt    = TensorType::FromString(R"(-1, 0, 3, 0, -8, 0, -21, 0)");

  fetch::ml::ops::Relu<TensorType> op;
  std::vector<TensorType>          prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt));
}

TYPED_TEST(ReluTest, backward_3d_tensor_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType       data({2, 2, 2});
  TensorType       error({2, 2, 2});
  TensorType       gt({2, 2, 2});
  std::vector<int> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> errorInput({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int> gt_input({-1, 0, 3, 0, -8, 0, -21, 0});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      for (SizeType k{0}; k < 2; ++k)
      {
        data.Set(i, j, k, fetch::math::AsType<DataType>(data_input[i + 2 * (j + 2 * k)]));
        error.Set(i, j, k, fetch::math::AsType<DataType>(errorInput[i + 2 * (j + 2 * k)]));
        gt.Set(i, j, k, fetch::math::AsType<DataType>(gt_input[i + 2 * (j + 2 * k)]));
      }
    }
  }

  fetch::ml::ops::Relu<TensorType> op;
  std::vector<TensorType>          prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::Type<DataType>("0.00001"),
                                     fetch::math::Type<DataType>("0.00001")));
}

TYPED_TEST(ReluTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TypeParam>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Relu<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::Relu<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8");
  TensorType gt   = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8");

  fetch::ml::ops::Relu<TensorType> op;
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

TYPED_TEST(ReluTest, saveparams_backward_3d_tensor_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;
  using OpType     = typename fetch::ml::ops::Relu<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType       data({2, 2, 2});
  TensorType       error({2, 2, 2});
  TensorType       gt({2, 2, 2});
  std::vector<int> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> errorInput({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int> gt_input({-1, 0, 3, 0, -8, 0, -21, 0});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      for (SizeType k{0}; k < 2; ++k)
      {
        data.Set(i, j, k, fetch::math::AsType<DataType>(data_input[i + 2 * (j + 2 * k)]));
        error.Set(i, j, k, fetch::math::AsType<DataType>(errorInput[i + 2 * (j + 2 * k)]));
        gt.Set(i, j, k, fetch::math::AsType<DataType>(gt_input[i + 2 * (j + 2 * k)]));
      }
    }
  }

  fetch::ml::ops::Relu<TensorType> op;

  // run op once to make sure caches etc. have been filled. Otherwise the test might be trivial!
  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}
}  // namespace test
}  // namespace ml
}  // namespace fetch
