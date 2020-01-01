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

#include "ml/model/dnn_classifier.hpp"

#include "gtest/gtest.h"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/model/model.hpp"
#include "ml/saveparams/saveable_params.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"

namespace fetch {
namespace ml {
namespace test {
using SizeVector = fetch::math::SizeVector;

template <typename T>
class DNNClassifierModelTest : public ::testing::Test
{
};

TYPED_TEST_CASE(DNNClassifierModelTest, math::test::HighPrecisionTensorFloatingTypes);

namespace classifier_details {
template <typename TypeParam>
void PrepareTestDataAndLabels1D(TypeParam &train_data, TypeParam &train_label)
{
  train_data  = TypeParam::FromString("0, 1, 0; 1, 0, 0; 0, 0, 1");
  train_label = TypeParam::FromString("0, 0, 1; 0, 1, 0; 1, 0, 0");
}

template <typename TypeParam, typename DataType, typename ModelType>
ModelType SetupModel(fetch::ml::OptimiserType                 optimiser_type,
                     fetch::ml::model::ModelConfig<DataType> &model_config, TypeParam &data,
                     TypeParam &gt)
{
  // setup dataloader
  using DataLoaderType = fetch::ml::dataloaders::TensorDataLoader<TypeParam, TypeParam>;
  auto data_loader_ptr = std::make_unique<DataLoaderType>();
  data_loader_ptr->AddData({data}, gt);

  // run model in training mode
  auto model = ModelType(model_config, {3, 30, 30, 3});
  model.SetDataloader(std::move(data_loader_ptr));
  model.Compile(optimiser_type, ops::LossType::NONE, {ops::MetricType::CATEGORICAL_ACCURACY});

  return model;
}

template <typename TypeParam>
bool RunTest(fetch::ml::OptimiserType optimiser_type, typename TypeParam::Type tolerance,
             typename TypeParam::Type lr = fetch::math::Type<typename TypeParam::Type>("0.5"),
             fetch::math::SizeType    training_steps = 100)
{
  using DataType  = typename TypeParam::Type;
  using ModelType = fetch::ml::model::DNNClassifier<TypeParam>;

  fetch::ml::model::ModelConfig<DataType> model_config;
  model_config.learning_rate_param.mode =
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::EXPONENTIAL;
  model_config.learning_rate_param.starting_learning_rate = lr;
  model_config.learning_rate_param.exponential_decay_rate = fetch::math::Type<DataType>("0.99");

  // set up data
  TypeParam train_data, train_labels;
  PrepareTestDataAndLabels1D<TypeParam>(train_data, train_labels);

  // set up model
  ModelType model = SetupModel<TypeParam, DataType, ModelType>(optimiser_type, model_config,
                                                               train_data, train_labels);

  // test loss decreases
  DataType loss{0};
  DataType later_loss{0};

  model.Train(1, loss);
  model.Train(training_steps);
  model.Train(1, later_loss);

  EXPECT_LE(later_loss, loss);

  // test prediction performance
  TypeParam pred;
  model.Predict(train_data, pred);
  EXPECT_TRUE(pred.AllClose(train_labels, tolerance, tolerance));

  typename model::Model<TypeParam>::DataVectorType eval =
      model.Evaluate(dataloaders::DataLoaderMode::TRAIN);

  EXPECT_EQ(eval.size(), 2);

  // check loss
  auto double_tolerance = static_cast<double>(tolerance);
  EXPECT_NEAR(static_cast<double>(eval[0]), 0, double_tolerance);

  // check accuracy
  EXPECT_NEAR(static_cast<double>(eval[1]), 1, double_tolerance);

  return true;
}

}  // namespace classifier_details

TYPED_TEST(DNNClassifierModelTest, adagrad_dnnclasifier)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(classifier_details::RunTest<TypeParam>(fetch::ml::OptimiserType::ADAGRAD,
                                                     fetch::math::Type<DataType>("0.01"),
                                                     fetch::math::Type<DataType>("0.03"), 400));
}

TYPED_TEST(DNNClassifierModelTest, adam_dnnclasifier)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(classifier_details::RunTest<TypeParam>(fetch::ml::OptimiserType::ADAM,
                                                     fetch::math::Type<DataType>("0.00001"),
                                                     fetch::math::Type<DataType>("0.1")));
}

TYPED_TEST(DNNClassifierModelTest, momentum_dnnclasifier)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(classifier_details::RunTest<TypeParam>(fetch::ml::OptimiserType::MOMENTUM,
                                                     fetch::math::Type<DataType>("0.00001")));
}

TYPED_TEST(DNNClassifierModelTest, rmsprop_dnnclasifier)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(classifier_details::RunTest<TypeParam>(fetch::ml::OptimiserType::RMSPROP,
                                                     fetch::math::Type<DataType>("0.0001"),
                                                     fetch::math::Type<DataType>("0.004"), 400));
}

TYPED_TEST(DNNClassifierModelTest, sgd_dnnclasifier)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(classifier_details::RunTest<TypeParam>(fetch::ml::OptimiserType::SGD,
                                                     fetch::math::Type<DataType>("0.01"),
                                                     fetch::math::Type<DataType>("0.7"), 400));
}

TYPED_TEST(DNNClassifierModelTest, sgd_dnnclasifier_serialisation)
{
  using DataType  = typename TypeParam::Type;
  using ModelType = fetch::ml::model::DNNClassifier<TypeParam>;

  fetch::math::SizeType    n_training_steps = 10;
  auto                     tolerance        = DataType{0};
  auto                     learning_rate    = fetch::math::Type<DataType>("0.06");
  fetch::ml::OptimiserType optimiser_type   = fetch::ml::OptimiserType::SGD;

  fetch::ml::model::ModelConfig<DataType> model_config;
  model_config.learning_rate_param.mode =
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::EXPONENTIAL;
  model_config.learning_rate_param.starting_learning_rate = learning_rate;
  model_config.learning_rate_param.exponential_decay_rate = fetch::math::Type<DataType>("0.99");

  // set up data
  TypeParam train_data, train_labels, test_datum, test_label;
  classifier_details::PrepareTestDataAndLabels1D<TypeParam>(train_data, train_labels);

  // set up model
  ModelType model = classifier_details::SetupModel<TypeParam, DataType, ModelType>(
      optimiser_type, model_config, train_data, train_labels);

  // test prediction performance
  TypeParam pred1({3, 3});
  TypeParam pred2({3, 3});

  // serialise the model
  fetch::serializers::MsgPackSerializer b;
  b << model;

  // deserialise the model
  b.seek(0);
  auto model2 = std::make_shared<fetch::ml::model::DNNClassifier<TypeParam>>();
  b >> *model2;

  model.Predict(train_data, pred1);
  model2->Predict(train_data, pred2);

  // Test if deserialised model returns same results
  EXPECT_TRUE(pred1.AllClose(pred2, tolerance, tolerance));

  // Do one iteration step
  model2->Train(n_training_steps);
  model2->Predict(train_data, pred1);

  // Test if only one model is being trained
  EXPECT_FALSE(pred1.AllClose(pred2, tolerance, tolerance));

  model.Train(n_training_steps);
  model.Predict(train_data, pred2);

  // Test if both models returns same results after training
  EXPECT_TRUE(pred1.AllClose(pred2, tolerance, tolerance));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
