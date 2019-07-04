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

#include "math/tensor.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/estimators/dnn_classifier.hpp"

#include "gtest/gtest.h"

template <typename T>
class EstimatorsTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(EstimatorsTest, MyTypes);

template <typename TypeParam>
void PrepareTestDataAndLabels1D(TypeParam &data, TypeParam &gt, TypeParam &test_datum)
{
  data       = TypeParam::FromString("0, 1, 0; 1, 0, 0; 0, 0, 1");
  gt         = TypeParam::FromString("0, 1, 0; 1, 0, 0; 0, 0, 1");
  test_datum = TypeParam::FromString("0; 1; 0");
}

template <typename TypeParam, typename DataType, typename EstimatorType>
EstimatorType SetupEstimator(fetch::ml::estimator::EstimatorConfig<DataType> &estimator_config,
                             TypeParam &data, TypeParam &gt)
{
  // setup dataloader
  using DataLoaderType = fetch::ml::dataloaders::TensorDataLoader<TypeParam, TypeParam>;
  auto data_loader_ptr = std::make_shared<DataLoaderType>();
  data_loader_ptr->AddData(data, gt);

  // run estimator in training mode
  return EstimatorType(estimator_config, data_loader_ptr, {3, 100, 100, 3});
}

template <typename TypeParam>
bool RunTest(fetch::ml::optimisers::OptimiserType optimiser_type)
{
  using DataType      = typename TypeParam::Type;
  using EstimatorType = fetch::ml::estimator::DNNClassifier<TypeParam>;

  fetch::math::SizeType n_training_steps = 10;

  fetch::ml::estimator::EstimatorConfig<DataType> estimator_config;
  estimator_config.learning_rate_param.mode                   = fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::EXPONENTIAL;
  estimator_config.learning_rate_param.starting_learning_rate = static_cast<DataType>(0.1);
  estimator_config.learning_rate_param.exponential_decay_rate = static_cast<DataType>(0.99);
  estimator_config.opt           = optimiser_type;

  // set up data
  TypeParam data, gt, test_datum;
  PrepareTestDataAndLabels1D<TypeParam>(data, gt, test_datum);

  // set up estimator
  EstimatorType estimator =
      SetupEstimator<TypeParam, DataType, EstimatorType>(estimator_config, data, gt);

  // test loss decreases
  fetch::math::SizeType count{0};
  while (count < n_training_steps)
  {
    DataType loss{0};
    DataType later_loss{0};
    EXPECT_TRUE(estimator.Train(1, loss));
    EXPECT_TRUE(estimator.Train(1, later_loss));
    count++;
  }

  EXPECT_TRUE(estimator.Train(100));

  // test prediction performance
  TypeParam pred({3, 1});

  estimator.Train(100);
  bool success = estimator.Predict(test_datum, pred);

  EXPECT_TRUE(success);
  EXPECT_TRUE(pred.AllClose(test_datum, static_cast<DataType>(1e-5), static_cast<DataType>(1e-5)));

  return true;
}

TYPED_TEST(EstimatorsTest, adagrad_dnnclasifier)
{
  ASSERT_TRUE(RunTest<TypeParam>(fetch::ml::optimisers::OptimiserType::ADAGRAD));
}
TYPED_TEST(EstimatorsTest, adam_dnnclasifier)
{
  ASSERT_TRUE(RunTest<TypeParam>(fetch::ml::optimisers::OptimiserType::ADAM));
}
TYPED_TEST(EstimatorsTest, momentum_dnnclasifier)
{
  ASSERT_TRUE(RunTest<TypeParam>(fetch::ml::optimisers::OptimiserType::MOMENTUM));
}
TYPED_TEST(EstimatorsTest, rmsprop_dnnclasifier)
{
  ASSERT_TRUE(RunTest<TypeParam>(fetch::ml::optimisers::OptimiserType::RMSPROP));
}
TYPED_TEST(EstimatorsTest, sgd_dnnclasifier)
{
  ASSERT_TRUE(RunTest<TypeParam>(fetch::ml::optimisers::OptimiserType::SGD));
}
