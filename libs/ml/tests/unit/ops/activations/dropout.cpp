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

#include "core/serializers/main_serializer_definition.hpp"
#include "math/base_types.hpp"
#include "test_types.hpp"
#include "ml/ops/activations/dropout.hpp"
#include "ml/serializers/ml_types.hpp"
#include "gtest/gtest.h"
#include <memory>

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class DropoutTest : public ::testing::Test
{
};

TYPED_TEST_CASE(DropoutTest, math::test::TensorFloatingTypes);

TYPED_TEST(DropoutTest, forward_test)
{
  using DataType      = typename TypeParam::Type;
  using TensorType    = TypeParam;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  TensorType data = TensorType::FromString(R"(1, -2, 3, -4, 5, -6, 7, -8)");
  TensorType gt   = TensorType::FromString(R"(0, -2, 0,  0, 5, -6, 7, -8)");
  DataType   prob{0.5};

  fetch::math::Multiply(gt, DataType{1} / prob, gt);

  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));

  // Test after generating new random alpha value
  gt = TensorType::FromString(R"(1, -2, 3, 0, 5, 0, 7, 0)");
  fetch::math::Multiply(gt, DataType{1} / prob, gt);

  op.Forward(VecTensorType({std::make_shared<const TensorType>(data)}), prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));

  // Test with is_training set to false
  op.SetTraining(false);

  gt = TensorType::FromString(R"(1, -2, 3, -4, 5, -6, 7, -8)");

  op.Forward(VecTensorType({std::make_shared<const TensorType>(data)}), prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(DropoutTest, forward_3d_tensor_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({2, 2, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> gt_input({0, -2, 0, 0, 5, -6, 7, -8});
  DataType            prob{0.5};

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
  fetch::math::Multiply(gt, DataType{1} / prob, gt);

  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);
  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(DropoutTest, backward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data  = TensorType::FromString(R"(1, -2, 3, -4, 5, -6, 7, -8)");
  TensorType error = TensorType::FromString(R"(0, 0, 0, 0, 1, 1, 0, 0)");
  TensorType gt    = TensorType::FromString(R"(0, 0, 0, 0, 1, 1, 0, 0)");
  DataType   prob{0.5};
  fetch::math::Multiply(gt, DataType{1} / prob, gt);

  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);

  // It's necessary to do Forward pass first
  TensorType output(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, output);

  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));

  // Test after generating new random alpha value
  // Forward pass will update random value
  op.Forward({std::make_shared<const TensorType>(data)}, output);

  gt = TensorType::FromString(R"(0, 0, 0, 0, 1, 0, 0, 0)");
  fetch::math::Multiply(gt, DataType{1} / prob, gt);
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(DropoutTest, backward_3d_tensor_test)
{
  using DataType      = typename TypeParam::Type;
  using TensorType    = TypeParam;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  DataType prob{0.5};

  TensorType          data({2, 2, 2});
  TensorType          error({2, 2, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> error_input({0, 0, 0, 0, 1, 1, 0, 0});
  std::vector<double> gt_input({0, 0, 0, 0, 1, 1, 0, 0});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      for (SizeType k{0}; k < 2; ++k)
      {
        data.Set(i, j, k, static_cast<DataType>(data_input[i + 2 * (j + 2 * k)]));
        error.Set(i, j, k, static_cast<DataType>(error_input[i + 2 * (j + 2 * k)]));
        gt.Set(i, j, k, static_cast<DataType>(gt_input[i + 2 * (j + 2 * k)]));
      }
    }
  }
  fetch::math::Multiply(gt, DataType{1} / prob, gt);

  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);

  // It's necessary to do Forward pass first
  TensorType output(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward(VecTensorType({std::make_shared<const TensorType>(data)}), output);

  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(DropoutTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Dropout<TensorType>::SPType;
  using OpType        = fetch::ml::ops::Dropout<TensorType>;

  TensorType                    data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TensorType                    gt   = TensorType::FromString("0, -2, 0,  0, 5, -6, 7, -8");
  DataType                      prob{0.5};
  typename TensorType::SizeType random_seed = 12345;

  fetch::math::Multiply(gt, DataType{1} / prob, gt);

  OpType op(prob, random_seed);

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

  // make another prediction with the original graph
  op.Forward(vec_data, prediction);

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
  EXPECT_TRUE(
      new_prediction.AllClose(prediction, static_cast<DataType>(0), static_cast<DataType>(0)));
}

TYPED_TEST(DropoutTest, saveparams_backward_3d_tensor_test)
{
  using DataType      = typename TypeParam::Type;
  using TensorType    = TypeParam;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using OpType        = fetch::ml::ops::Dropout<TensorType>;
  using SPType        = typename OpType::SPType;
  DataType prob{0.5};

  TensorType          data({2, 2, 2});
  TensorType          error({2, 2, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> error_input({0, 0, 0, 0, 1, 1, 0, 0});
  std::vector<double> gt_input({0, 0, 0, 0, 1, 1, 0, 0});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      for (SizeType k{0}; k < 2; ++k)
      {
        data.Set(i, j, k, static_cast<DataType>(data_input[i + 2 * (j + 2 * k)]));
        error.Set(i, j, k, static_cast<DataType>(error_input[i + 2 * (j + 2 * k)]));
        gt.Set(i, j, k, static_cast<DataType>(gt_input[i + 2 * (j + 2 * k)]));
      }
    }
  }
  fetch::math::Multiply(gt, DataType{1} / prob, gt);

  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);

  // It's necessary to do Forward pass first
  TensorType output(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward(VecTensorType({std::make_shared<const TensorType>(data)}), output);

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
  op.Forward(VecTensorType({std::make_shared<const TensorType>(data)}), output);
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // must call forward again to populate internal caches before backward can be called
  new_op.Forward(VecTensorType({std::make_shared<const TensorType>(data)}), output);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}


} // test
} // ml
} // fetch