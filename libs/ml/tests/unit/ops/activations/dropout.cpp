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

#include "test_types.hpp"

#include "core/serializers/main_serializer_definition.hpp"
#include "math/base_types.hpp"
#include "math/standard_functions/sqrt.hpp"
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

TYPED_TEST_SUITE(DropoutTest, math::test::TensorFloatingTypes, );

namespace {
template <typename TensorType>
typename TensorType::Type zero_fraction(TensorType const &t1)
{
  using DataType = typename TensorType::Type;
  DataType ret{0};
  for (auto const &i : t1)
  {
    if (i == DataType{0})
    {
      ret++;
    }
  }
  return ret / static_cast<DataType>(t1.size());
}

/**
 * This calculates 2stdev for zero_fraction
 * zero_fraction is the sum of n samples from a binomial distribution, divided by n
 * @param tensorsize number of samples
 * @param prob binomial distribution probability
 * @return
 */
template <typename DataType>
DataType two_stdev(math::SizeType tensorsize, DataType prob)
{
  return DataType{2} *
         fetch::math::Sqrt(static_cast<DataType>(tensorsize) * prob * (DataType{1} - prob)) /
         static_cast<DataType>(tensorsize);
}

}  // namespace

TYPED_TEST(DropoutTest, forward_test)
{
  using DataType      = typename TypeParam::Type;
  using TensorType    = TypeParam;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  math::SizeType tensorsize = 10000;
  TensorType     data       = TensorType(tensorsize);
  data.Fill(DataType{1});
  auto prob = fetch::math::Type<DataType>("0.2");

  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);

  TensorType    prediction(op.ComputeOutputShape({data.shape()}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // test correct fraction and sum
  DataType abs_error = two_stdev(tensorsize, static_cast<DataType>(prob));
  EXPECT_NEAR(static_cast<double>(zero_fraction(prediction)), static_cast<double>(prob),
              static_cast<double>(abs_error));
  // using 2* two_stdev * tensorsize for the error on this calculation is not quite correct but it's
  // close enough
  EXPECT_NEAR(static_cast<double>(fetch::math::Sum(prediction)),
              static_cast<double>(fetch::math::Sum(data)),
              static_cast<double>(DataType{2} * abs_error * static_cast<DataType>(tensorsize)));

  // Test after generating new random alpha value
  op.Forward(VecTensorType({std::make_shared<const TensorType>(data)}), prediction);

  // test correct fraction and sum
  EXPECT_NEAR(static_cast<double>(zero_fraction(prediction)), static_cast<double>(prob),
              static_cast<double>(abs_error));
  EXPECT_NEAR(static_cast<double>(fetch::math::Sum(prediction)),
              static_cast<double>(fetch::math::Sum(data)),
              static_cast<double>(DataType{2} * abs_error * static_cast<DataType>(tensorsize)));

  // Test with is_training set to false
  op.SetTraining(false);

  op.Forward(VecTensorType({std::make_shared<const TensorType>(data)}), prediction);

  // test correct fraction and values
  EXPECT_EQ(zero_fraction(prediction), DataType{0});
  EXPECT_TRUE(prediction.AllClose(data));
}

TYPED_TEST(DropoutTest, forward_3d_tensor_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  math::SizeType tensorsize = 1000;
  TensorType     data       = TensorType(tensorsize);
  data.Fill(DataType{1});
  data.Reshape({10, 10, 10});
  DataType prob = fetch::math::Type<DataType>("0.3");

  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);
  TypeParam                           prediction(op.ComputeOutputShape({data.shape()}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct fraction, sum and shape
  DataType abs_error = two_stdev(tensorsize, static_cast<DataType>(prob));
  EXPECT_NEAR(static_cast<double>(zero_fraction(prediction)), static_cast<double>(prob),
              static_cast<double>(abs_error));
  EXPECT_NEAR(static_cast<double>(fetch::math::Sum(prediction)),
              static_cast<double>(fetch::math::Sum(data)),
              static_cast<double>(DataType{2} * abs_error * static_cast<DataType>(tensorsize)));
  EXPECT_EQ(prediction.shape(), data.shape());
}

TYPED_TEST(DropoutTest, backward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  math::SizeType tensorsize = 10000;
  TensorType     data       = TensorType(tensorsize);
  data.Fill(DataType{1});
  TensorType error = TensorType(tensorsize);
  error.Fill(DataType{1});
  auto const prob = fetch::math::Type<DataType>("0.2");

  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);

  // It's necessary to do a forward pass first
  TensorType output(op.ComputeOutputShape({data.shape()}));
  op.Forward({std::make_shared<const TensorType>(data)}, output);

  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct fraction and sum
  DataType abs_error = two_stdev(tensorsize, static_cast<DataType>(prob));
  EXPECT_NEAR(static_cast<double>(zero_fraction(prediction[0])), static_cast<double>(prob),
              static_cast<double>(abs_error));
  EXPECT_NEAR(static_cast<double>(fetch::math::Sum(prediction[0])),
              static_cast<double>(fetch::math::Sum(error)),
              static_cast<double>(DataType{2} * abs_error * static_cast<DataType>(tensorsize)));

  // Test after generating new random alpha value
  // Forward pass will update random value
  op.Forward({std::make_shared<const TensorType>(data)}, output);

  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct fraction and sum
  EXPECT_NEAR(static_cast<double>(zero_fraction(prediction[0])), static_cast<double>(prob),
              static_cast<double>(abs_error));
  EXPECT_NEAR(static_cast<double>(fetch::math::Sum(prediction[0])),
              static_cast<double>(fetch::math::Sum(error)),
              static_cast<double>(DataType{2} * abs_error * static_cast<DataType>(tensorsize)));
}

TYPED_TEST(DropoutTest, backward_3d_tensor_test)
{
  using DataType      = typename TypeParam::Type;
  using TensorType    = TypeParam;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  DataType prob       = fetch::math::Type<DataType>("0.2");

  math::SizeType tensorsize = 1000;
  TensorType     data       = TensorType(tensorsize);
  data.Fill(DataType{1});
  data.Reshape({10, 10, 10});

  TensorType error = TensorType(tensorsize);
  error.Fill(DataType{1});
  error.Reshape({10, 10, 10});

  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);

  // It's necessary to do a forward pass first
  TensorType output(op.ComputeOutputShape({data.shape()}));
  op.Forward(VecTensorType({std::make_shared<const TensorType>(data)}), output);

  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct fraction, sum and shape
  DataType abs_error = two_stdev(tensorsize, static_cast<DataType>(prob));
  EXPECT_NEAR(static_cast<double>(zero_fraction(prediction[0])), static_cast<double>(prob),
              static_cast<double>(abs_error));
  EXPECT_NEAR(static_cast<double>(fetch::math::Sum(prediction[0])),
              static_cast<double>(fetch::math::Sum(error)),
              static_cast<double>(DataType{2} * abs_error * static_cast<DataType>(tensorsize)));
  EXPECT_EQ(prediction[0].shape(), error.shape());
}

TYPED_TEST(DropoutTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Dropout<TensorType>::SPType;
  using OpType        = fetch::ml::ops::Dropout<TensorType>;

  math::SizeType tensorsize = 1000;
  TensorType     data       = TensorType(tensorsize);
  data.Fill(DataType{1});
  auto const                    prob        = fetch::math::Type<DataType>("0.3");
  typename TensorType::SizeType random_seed = 12345;

  OpType op(prob, random_seed);

  TensorType    prediction(op.ComputeOutputShape({data.shape()}));
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
  TensorType new_prediction(op.ComputeOutputShape({data.shape()}));
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
  TensorType     data       = TensorType(tensorsize);
  data.Fill(DataType{1});
  data.Reshape({10, 10, 10});
  TensorType error = TensorType(tensorsize);
  error.Fill(DataType{2});
  error.Reshape({10, 10, 10});
  fetch::ml::ops::Dropout<TensorType> op(prob, 12345);

  // It's necessary to do Forward pass first
  TensorType output(op.ComputeOutputShape({data.shape()}));
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
