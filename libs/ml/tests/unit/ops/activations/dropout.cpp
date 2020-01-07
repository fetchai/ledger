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
#include "ml/ops/activations/dropout.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"

#include <memory>

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class DropoutTest : public ::testing::Test
{
};

TYPED_TEST_CASE(DropoutTest, math::test::TensorFloatingTypes);

namespace {
template <typename TensorType>
double zero_fraction(TensorType const &t1)
{
  using DataType = typename TensorType::Type;
  double ret     = 0;
  for (auto const &i : t1)
  {
    if (i == DataType{0})
    {
      ret++;
    }
  }
  return ret / static_cast<double>(t1.size());
}

/**
 * This calculates 2stdev for zero_fraction
 * zero_fraction is the sum of n samples from a binomial distribution, divided by n
 * @param tensorsize number of samples
 * @param prob binomial distribution probability
 * @return
 */
double two_stdev(math::SizeType tensorsize, double prob)
{
  return 2.0 * sqrt(static_cast<double>(tensorsize) * prob * (1.0 - prob)) /
         static_cast<double>(tensorsize);
}

}  // namespace

TYPED_TEST(DropoutTest, forward_test)
{
  using DataType      = typename TypeParam::Type;
  using TensorType    = TypeParam;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  math::SizeType tensorsize = 10000;
  TensorType     data       = TensorType::UniformRandom(tensorsize);
  auto           prob       = fetch::math::Type<DataType>("0.5");

  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // test correct fraction and sum
  double abs_error = two_stdev(tensorsize, static_cast<double>(prob));
  EXPECT_NEAR(zero_fraction(prediction), static_cast<double>(prob), abs_error);
  // using 2* two_stdev * tensorsize for the error on this calculation is not quite correct but it's
  // close enough
  EXPECT_NEAR(static_cast<double>(fetch::math::Sum(prediction)),
              static_cast<double>(fetch::math::Sum(data)),
              2 * abs_error * static_cast<double>(tensorsize));

  // Test after generating new random alpha value
  op.Forward(VecTensorType({std::make_shared<const TensorType>(data)}), prediction);

  // test correct fraction and sum
  EXPECT_NEAR(zero_fraction(prediction), static_cast<double>(prob), abs_error);
  EXPECT_NEAR(static_cast<double>(fetch::math::Sum(prediction)),
              static_cast<double>(fetch::math::Sum(data)),
              2 * abs_error * static_cast<double>(tensorsize));

  // Test with is_training set to false
  op.SetTraining(false);

  op.Forward(VecTensorType({std::make_shared<const TensorType>(data)}), prediction);

  // test correct fraction and values
  EXPECT_EQ(zero_fraction(prediction), 0);
  EXPECT_TRUE(prediction.AllClose(data));
}

TYPED_TEST(DropoutTest, forward_3d_tensor_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  math::SizeType tensorsize = 1000;
  TensorType     data       = TensorType::UniformRandom(tensorsize);
  data.Reshape({10, 10, 10});
  DataType prob = fetch::math::Type<DataType>("0.5");

  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);
  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct fraction, sum and shape
  double abs_error = two_stdev(tensorsize, static_cast<double>(prob));
  EXPECT_NEAR(zero_fraction(prediction), static_cast<double>(prob), abs_error);
  EXPECT_NEAR(static_cast<double>(fetch::math::Sum(prediction)),
              static_cast<double>(fetch::math::Sum(data)),
              2 * abs_error * static_cast<double>(tensorsize));
  EXPECT_EQ(prediction.shape(), data.shape());
}

TYPED_TEST(DropoutTest, backward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  math::SizeType tensorsize = 10000;
  TensorType     data       = TensorType::UniformRandom(tensorsize);
  TensorType     error      = TensorType::UniformRandom(tensorsize);
  auto const     prob       = fetch::math::Type<DataType>("0.5");

  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);

  // It's necessary to do a forward pass first
  TensorType output(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, output);

  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct fraction and sum
  double abs_error = two_stdev(tensorsize, static_cast<double>(prob));
  EXPECT_NEAR(zero_fraction(prediction[0]), static_cast<double>(prob), abs_error);
  EXPECT_NEAR(static_cast<double>(fetch::math::Sum(prediction[0])),
              static_cast<double>(fetch::math::Sum(error)),
              2 * abs_error * static_cast<double>(tensorsize));

  // Test after generating new random alpha value
  // Forward pass will update random value
  op.Forward({std::make_shared<const TensorType>(data)}, output);

  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct fraction and sum
  EXPECT_NEAR(zero_fraction(prediction[0]), static_cast<double>(prob), abs_error);
  EXPECT_NEAR(static_cast<double>(fetch::math::Sum(prediction[0])),
              static_cast<double>(fetch::math::Sum(error)),
              2 * abs_error * static_cast<double>(tensorsize));
}

TYPED_TEST(DropoutTest, backward_3d_tensor_test)
{
  using DataType      = typename TypeParam::Type;
  using TensorType    = TypeParam;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  DataType prob       = fetch::math::Type<DataType>("0.5");

  math::SizeType tensorsize = 1000;
  TensorType     data       = TensorType::UniformRandom(tensorsize);
  data.Reshape({10, 10, 10});

  TensorType error = TensorType::UniformRandom(tensorsize);
  error.Reshape({10, 10, 10});

  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);

  // It's necessary to do a forward pass first
  TensorType output(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward(VecTensorType({std::make_shared<const TensorType>(data)}), output);

  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct fraction, sum and shape
  double abs_error = two_stdev(tensorsize, static_cast<double>(prob));
  EXPECT_NEAR(zero_fraction(prediction[0]), static_cast<double>(prob), abs_error);
  EXPECT_NEAR(static_cast<double>(fetch::math::Sum(prediction[0])),
              static_cast<double>(fetch::math::Sum(error)),
              2 * abs_error * static_cast<double>(tensorsize));
  EXPECT_EQ(prediction[0].shape(), error.shape());
}

TYPED_TEST(DropoutTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Dropout<TensorType>::SPType;
  using OpType        = fetch::ml::ops::Dropout<TensorType>;

  math::SizeType                tensorsize  = 1000;
  TensorType                    data        = TensorType::UniformRandom(tensorsize);
  auto const                    prob        = fetch::math::Type<DataType>("0.5");
  typename TensorType::SizeType random_seed = 12345;

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
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(DropoutTest, saveparams_backward_3d_tensor_test)
{
  using DataType      = typename TypeParam::Type;
  using TensorType    = TypeParam;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using OpType        = fetch::ml::ops::Dropout<TensorType>;
  using SPType        = typename OpType::SPType;
  DataType prob       = fetch::math::Type<DataType>("0.5");

  math::SizeType tensorsize = 1000;
  TensorType     data       = TensorType::UniformRandom(tensorsize);
  data.Reshape({10, 10, 10});
  TensorType error = TensorType::UniformRandom(tensorsize);
  error.Reshape({10, 10, 10});
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

}  // namespace test
}  // namespace ml
}  // namespace fetch
