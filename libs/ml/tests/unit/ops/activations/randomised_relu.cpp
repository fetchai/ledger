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

#include "core/serializers/main_serializer.hpp"
#include "gtest/gtest.h"
#include "math/base_types.hpp"
#include "math/standard_functions/abs.hpp"
#include "ml/ops/activations/randomised_relu.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class RandomisedReluTest : public ::testing::Test
{
};

TYPED_TEST_CASE(RandomisedReluTest, math::test::TensorFloatingTypes);

/**
 * @tparam DataType
 * @param a
 * @param b
 * @param lower_bound
 * @param upper_bound
 * @return bool if abs(a)>=abs(b*lower) and abs(a)<=abs(b*upper)
 */
template <typename DataType>
bool IsAbsWithinRange(DataType a, DataType b, DataType lower_bound, DataType upper_bound)
{
  return (fetch::math::Abs(a) >= fetch::math::Abs(b * lower_bound)) &&
         (fetch::math::Abs(a) <= fetch::math::Abs(b * upper_bound));
}

/**
 * Function for forward pass tests
 * Checks if RandomRelu is applied for negative values only
 * @tparam TypeParam
 * @param data
 * @param prediction
 * @param lower_bound
 * @param upper_bound
 */
template <typename TypeParam>
void CheckForwardValues(TypeParam &data, TypeParam &prediction,
                        typename TypeParam::Type lower_bound, typename TypeParam::Type upper_bound)
{
  auto data_it = data.begin();
  auto pred_it = prediction.begin();
  while (data_it.is_valid())
  {
    if (*data_it < typename TypeParam::Type{0})
    {
      EXPECT_TRUE(IsAbsWithinRange(*pred_it, *data_it, lower_bound, upper_bound));
    }
    else
    {
      EXPECT_TRUE(*pred_it == *data_it);
    }
    ++data_it;
    ++pred_it;
  }
}

TYPED_TEST(RandomisedReluTest, forward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  DataType lower_bound = fetch::math::Type<DataType>("0.03");
  DataType upper_bound = fetch::math::Type<DataType>("0.08");

  fetch::ml::ops::RandomisedRelu<TensorType> op(fetch::math::Type<DataType>("0.03"),
                                                fetch::math::Type<DataType>("0.08"), 12345);
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test if values are within ranges
  CheckForwardValues(data, prediction, lower_bound, upper_bound);

  // Test after generating new random alpha value
  TensorType prediction_2 =
      TensorType(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction_2);

  // test if values changed
  EXPECT_FALSE(prediction_2.AllClose(prediction, math::function_tolerance<DataType>(),
                                     math::function_tolerance<DataType>()));

  // test if values are within ranges
  CheckForwardValues(data, prediction_2, lower_bound, upper_bound);

  // Test with is_training set to false
  op.SetTraining(false);

  TensorType gt = TensorType::FromString("1, -0.11, 3, -0.22, 5, -0.33, 7, -0.44");

  TensorType prediction_3 =
      TensorType(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, math::function_tolerance<DataType>(),
                                  math::function_tolerance<DataType>()));
}

TYPED_TEST(RandomisedReluTest, forward_3d_tensor_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  DataType lower_bound = fetch::math::Type<DataType>("0.03");
  DataType upper_bound = fetch::math::Type<DataType>("0.08");

  TensorType data = TensorType::FromString("1, 3, 5, 7; -2, -4, -6, -8;");
  data.Reshape({2, 2, 2});

  fetch::ml::ops::RandomisedRelu<TensorType> op(lower_bound, upper_bound, 12345);
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test if values are within ranges
  CheckForwardValues(data, prediction, lower_bound, upper_bound);
}

TYPED_TEST(RandomisedReluTest, backward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  DataType lower_bound = fetch::math::Type<DataType>("0.03");
  DataType upper_bound = fetch::math::Type<DataType>("0.08");

  TensorType data  = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TensorType error = TensorType::FromString("0, 0, 0, 0, 1, 1, 0, 0");
  fetch::ml::ops::RandomisedRelu<TensorType> op(lower_bound, upper_bound, 12345);
  std::vector<TensorType>                    prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test if values are within ranges

  EXPECT_TRUE(prediction[0].At(0, 0) == DataType{0});
  EXPECT_TRUE(prediction[0].At(0, 1) == DataType{0});
  EXPECT_TRUE(prediction[0].At(0, 2) == DataType{0});
  EXPECT_TRUE(prediction[0].At(0, 3) == DataType{0});
  EXPECT_TRUE(prediction[0].At(0, 4) == DataType{1});
  EXPECT_TRUE(prediction[0].At(0, 5) >= lower_bound && prediction[0].At(0, 5) <= upper_bound);
  EXPECT_TRUE(prediction[0].At(0, 6) == DataType{0});
  EXPECT_TRUE(prediction[0].At(0, 7) == DataType{0});

  // Test after generating new random alpha value
  // Forward pass will update random value
  TensorType output(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, output);

  std::vector<TensorType> prediction_2 =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test if values changed
  EXPECT_FALSE(prediction_2[0].AllClose(prediction[0], math::function_tolerance<DataType>(),
                                        math::function_tolerance<DataType>()));

  // test if values are within ranges
  EXPECT_TRUE(prediction_2[0].At(0, 0) == DataType{0});
  EXPECT_TRUE(prediction_2[0].At(0, 1) == DataType{0});
  EXPECT_TRUE(prediction_2[0].At(0, 2) == DataType{0});
  EXPECT_TRUE(prediction_2[0].At(0, 3) == DataType{0});
  EXPECT_TRUE(prediction_2[0].At(0, 4) == DataType{1});
  EXPECT_TRUE(prediction_2[0].At(0, 5) >= lower_bound && prediction_2[0].At(0, 5) <= upper_bound);
  EXPECT_TRUE(prediction_2[0].At(0, 6) == DataType{0});
  EXPECT_TRUE(prediction_2[0].At(0, 7) == DataType{0});

  // Test with is_training set to false
  op.SetTraining(false);

  TensorType gt = TensorType::FromString("0, 0, 0, 0, 1, 0.055, 0, 0");
  prediction    = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction[0].AllClose(gt, math::function_tolerance<DataType>(),
                                     math::function_tolerance<DataType>()));
}

TYPED_TEST(RandomisedReluTest, backward_3d_tensor_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  DataType lower_bound = fetch::math::Type<DataType>("0.03");
  DataType upper_bound = fetch::math::Type<DataType>("0.08");

  TensorType data = TensorType::FromString("1, 3, 5, 7;-2, -4, -6, -8;");
  data.Reshape({2, 2, 2});
  TensorType error = TensorType::FromString("0, 0, 1, 0;0, 0, 1, 0;");
  error.Reshape({2, 2, 2});

  fetch::ml::ops::RandomisedRelu<TensorType> op(lower_bound, upper_bound, 12345);
  std::vector<TensorType>                    prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test if values are within ranges
  EXPECT_TRUE(prediction[0].At(0, 0, 0) == DataType{0});
  EXPECT_TRUE(prediction[0].At(0, 0, 1) == DataType{1});
  EXPECT_TRUE(prediction[0].At(0, 1, 0) == DataType{0});
  EXPECT_TRUE(prediction[0].At(0, 1, 1) == DataType{0});
  EXPECT_TRUE(prediction[0].At(1, 0, 0) == DataType{0});
  EXPECT_TRUE(prediction[0].At(1, 0, 1) >= lower_bound && prediction[0].At(1, 0, 1) <= upper_bound);
  EXPECT_TRUE(prediction[0].At(1, 1, 0) == DataType{0});
  EXPECT_TRUE(prediction[0].At(1, 1, 1) == DataType{0});
}

TYPED_TEST(RandomisedReluTest, saveparams_test)
{
  using DataType      = typename TypeParam::Type;
  using TensorType    = TypeParam;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::RandomisedRelu<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::RandomisedRelu<TensorType>;

  DataType lower_bound = fetch::math::Type<DataType>("0.03");
  DataType upper_bound = fetch::math::Type<DataType>("0.08");

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  fetch::ml::ops::RandomisedRelu<TensorType> op(lower_bound, upper_bound, 12345);
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
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(RandomisedReluTest, saveparams_backward_3d_tensor_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::RandomisedRelu<TensorType>;
  using SPType     = typename OpType::SPType;

  DataType lower_bound = fetch::math::Type<DataType>("0.03");
  DataType upper_bound = fetch::math::Type<DataType>("0.08");

  TensorType data = TensorType::FromString("1, 3, 5, 7;-2, -4, -6, -8;");
  data.Reshape({2, 2, 2});
  TensorType error = TensorType::FromString("0, 0, 1, 0;0, 0, 1, 0;");
  error.Reshape({2, 2, 2});

  fetch::ml::ops::RandomisedRelu<TensorType> op(lower_bound, upper_bound, 12345);

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
